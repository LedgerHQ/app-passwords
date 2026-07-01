from time import sleep

from ledgered.devices import Device
from ragger.backend import BackendInterface
from ragger.navigator import NavInsID
from ragger.navigator.navigator import Navigator

# Reuse the same CustomNavInsID enum as the touch navigator so that test files
# can be shared across device families. The touch/ and nano/ directories are
# imported as top-level packages by pytest (no __init__.py at tests/functional/),
# so the import is absolute, not relative.
from touch.navigator import CustomNavInsID


# Keyboard layout on Nano, MODE_NONE (see nbgl_obj_keyboard_nanos.c):
#   - First a mode-choice screen is shown with three icons (lower, upper, digits)
#     mapped to selectedCharIndex 0, 1, 2. both_click commits to that mode.
#   - Once a mode is selected, selectedCharIndex cycles through keysByMode[mode];
#     the last three characters are backspace ('\b'), validate ('\n') and the
#     mode-switch icon ('\r' -- returns to MODE_NONE).
_KEY_MAPS = (
    "abcdefghijklmnopqrstuvwxyz\b\n\r",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ\b\n\r",
    "0123456789 '\"`&/?!:;.,~*$=+-[](){}^<>\\_#@|%\b\n\r",
)
_MODE_LOWER = 0
_MODE_UPPER = 1
_MODE_DIGITS = 2


class CustomNanoNavigator(Navigator):
    """Button-based navigator for Nano S+ / Nano X.

    Layout assumptions (from src/ui/menu_nbgl.c):
      - Home (nbgl_useCaseHomeAndSettings) pages: Home, Action, Settings,
        Info x N, Quit (each reached via right_click; both_click confirms).
      - Main action menu (nbgl_useCaseNavigableContent BARS_LIST): one bar per
        page, in the order declared in barsTexts[]. After the bars comes the
        implicit "Back" entry.
      - Settings (nbgl_useCaseHomeAndSettings settings flow): one page per
        switch then keyboard layout choices then "Back".
      - Keyboard (nbgl_useCaseKeyboard with MODE_NONE): starts on a mode-choice
        screen (lower / upper / digits); see _KEY_MAPS for the per-mode rings.
        The navigator tracks (mode, selectedCharIndex) so each WRITE picks the
        shortest left/right path to the target character.
    """

    def __init__(
        self, backend: BackendInterface, device: Device, golden_run: bool = False
    ):
        self._backend = backend
        # Tracks the on-screen keyboard state between successive instructions.
        # `None` means the device is on the mode-choice screen (MODE_NONE) with
        # selectedCharIndex pointing at lower-case letters; once a mode is
        # picked, _kbd_mode holds its index (0/1/2) and _kbd_index follows
        # selectedCharIndex inside keysByMode[mode].
        self._kbd_mode: int | None = None
        self._kbd_index: int = 0

        callbacks = {
            NavInsID.WAIT: sleep,
            NavInsID.WAIT_FOR_SCREEN_CHANGE: backend.wait_for_screen_change,
            # First-launch disclaimer (nbgl_useCaseChoice): the message spans
            # several pages on Nano, so step through them before confirming.
            CustomNavInsID.DISCLAIMER_CONFIRM: self._disclaimer_confirm,
            # First-launch keyboard layout choice (CHOICES_LIST): QWERTY is the
            # first entry and shown by default.
            CustomNavInsID.CHOOSE_KBL_QWERTY: self._both,
            # Home -> action / settings / quit
            CustomNavInsID.HOME_TO_MENU: self._home_to_menu,
            CustomNavInsID.HOME_TO_SETTINGS: self._home_to_settings,
            # Settings navigation
            CustomNavInsID.SETTINGS_NEXT: self._right,
            CustomNavInsID.SETTINGS_TO_HOME: self._settings_to_home,
            # Main menu bars (see src/ui/menu_nbgl.c: Create, Type, Show,
            # Delete, Delete all). Each bar is a page on Nano.
            CustomNavInsID.MENU_TO_CREATE: lambda: self._menu_select_bar(0),
            CustomNavInsID.MENU_TO_TYPE: lambda: self._menu_select_bar(1),
            CustomNavInsID.MENU_TO_DISPLAY: lambda: self._menu_select_bar(2),
            CustomNavInsID.MENU_TO_DELETE: lambda: self._menu_select_bar(3),
            CustomNavInsID.MENU_TO_DELETE_ALL: lambda: self._menu_select_bar(4),
            # "Back" entry of the main menu
            CustomNavInsID.MENU_TO_HOME: self._menu_to_home,
            # Password list (CHOICES_LIST): LIST_CHOOSE(n) selects the n-th
            # entry (1-based, matching the touch navigator semantics).
            CustomNavInsID.LIST_CHOOSE: self._list_choose,
            CustomNavInsID.LIST_TO_MENU: self._list_to_menu,
            # Deletion confirmation (nbgl_useCaseChoice): on Nano the message
            # paginates before the "Confirm" action page, so step forward until
            # that page is on screen and only then commit.
            CustomNavInsID.CONFIRM_YES: self._confirm_yes,
            # Generic select/validate (settings switches, BARS_LIST entries).
            CustomNavInsID.BUTTON_APPROVE: self._both,
            # APDU approval prompt (nbgl_useCaseChoice): message page first,
            # the "Approve" action sits on the next page (right then both).
            CustomNavInsID.APDU_APPROVE: self._approve,
            # Same prompt, "Refuse" action: step forward until it shows, commit.
            CustomNavInsID.APDU_REJECT: self._reject,
            # Keyboard (nbgl_useCaseKeyboard with MODE_NONE on Nano).
            CustomNavInsID.KEYBOARD_WRITE: self._write,
            CustomNavInsID.KEYBOARD_TO_CONFIRM: self._keyboard_confirm,
        }
        super().__init__(backend, device, callbacks, golden_run=golden_run)

    # ---- low-level helpers -------------------------------------------------

    def _right(self):
        self._backend.right_click()

    def _left(self):
        self._backend.left_click()

    def _both(self):
        self._backend.both_click()

    def _approve(self):
        # nbgl_useCaseChoice shows the message first; the "Approve" action is
        # reached with one right_click, then validated with both_click. Matches
        # PasswordsManagerCommand.approve() for Nano.
        self._right()
        self._both()

    def _reject(self):
        # The "Refuse" action sits after "Approve" in the choice flow. Step
        # forward until it is on screen, then validate.
        self._navigate_until_text("Refuse")
        self._both()

    def _navigate_until_text(self, text: str, max_steps: int = 12) -> None:
        """Press right_click until `text` appears on screen, then return."""
        for _ in range(max_steps):
            try:
                self._backend.wait_for_text_on_screen(text, timeout=0.5)
                return
            except TimeoutError:
                self._backend.right_click()
        raise RuntimeError(
            f"Could not find text '{text}' on screen after {max_steps} steps"
        )

    # ---- first-launch disclaimer ------------------------------------------

    def _disclaimer_confirm(self):
        # The disclaimer message paginates across 4 screens before the confirm
        # action page is reached on Nano.
        for _ in range(4):
            self._right()
        self._both()

    # ---- home --------------------------------------------------------------

    def _home_to_menu(self):
        # Home page is page 0; Action ("Tap to manage") is page 1.
        self._right()
        self._both()

    def _home_to_settings(self):
        # Settings is page 2 from home.
        self._right()
        self._right()
        self._both()

    # ---- settings ----------------------------------------------------------

    def _settings_to_home(self):
        self._navigate_until_text("Back")
        self._both()

    # ---- main menu ---------------------------------------------------------

    def _menu_select_bar(self, index: int):
        # On entry the menu is at bar 0; navigate forward `index` times and
        # confirm.
        for _ in range(index):
            self._right()
        self._both()

    def _menu_to_home(self):
        # "Back" sits after all the bars; jump there with wait_for_text.
        self._navigate_until_text("Back")
        self._both()

    # ---- confirmation choice ----------------------------------------------

    def _confirm_yes(self):
        # The "Confirm the deletion ..." message is one page on Nano; the
        # standalone "Confirm" action page comes next. If a future message
        # ends up spanning multiple pages, raise the count of right_clicks
        # here (or switch to a wait_for_text loop).
        self._right()
        self._both()

    # ---- password list -----------------------------------------------------

    def _list_choose(self, position: int):
        # 1-based position to match the touch navigator's contract.
        for _ in range(position - 1):
            self._right()
        self._both()

    def _list_to_menu(self):
        self._navigate_until_text("Back")
        self._both()

    # ---- keyboard (MODE_NONE) ----------------------------------------------

    def _kbd_reset_state(self) -> None:
        """Drop the cached keyboard cursor (the device left the keyboard)."""
        self._kbd_mode = None
        self._kbd_index = 0

    def _kbd_navigate_to(self, target_index: int, ring_size: int) -> None:
        """Right- or left-click along the shortest path to `target_index`."""
        right_steps = (target_index - self._kbd_index) % ring_size
        left_steps = (self._kbd_index - target_index) % ring_size
        if right_steps <= left_steps:
            steps, action = right_steps, self._right
        else:
            steps, action = left_steps, self._left
        for _ in range(steps):
            action()
        self._kbd_index = target_index

    def _kbd_enter_mode(self, target_mode: int) -> None:
        """Select `target_mode` from the MODE_NONE screen."""
        self._kbd_navigate_to(target_mode, ring_size=3)
        self._both()
        self._kbd_mode = target_mode
        self._kbd_index = 0  # firmware resets cursor on mode entry

    def _kbd_leave_mode(self) -> None:
        """Press the mode-switch key ('\\r') to go back to MODE_NONE."""
        assert self._kbd_mode is not None
        keys = _KEY_MAPS[self._kbd_mode]
        self._kbd_navigate_to(len(keys) - 1, ring_size=len(keys))  # '\r'
        self._both()
        self._kbd_mode = None
        self._kbd_index = 0

    @staticmethod
    def _required_mode(char: str) -> int:
        if char.isupper():
            return _MODE_UPPER
        if char.islower():
            return _MODE_LOWER
        return _MODE_DIGITS  # digits + every supported symbol

    def _kbd_ensure_mode(self, target_mode: int) -> None:
        if self._kbd_mode == target_mode:
            return
        if self._kbd_mode is not None:
            self._kbd_leave_mode()
        self._kbd_enter_mode(target_mode)

    def _kbd_type_char(self, char: str) -> None:
        target_mode = self._required_mode(char)
        self._kbd_ensure_mode(target_mode)
        keys = _KEY_MAPS[self._kbd_mode]
        try:
            target_idx = keys.index(char)
        except ValueError as exc:
            raise ValueError(
                f"Character {char!r} cannot be typed in MODE_NONE keyboard"
            ) from exc
        self._kbd_navigate_to(target_idx, ring_size=len(keys))
        self._both()

    def _write(self, characters: str) -> None:
        for char in characters:
            self._kbd_type_char(char)

    def _keyboard_confirm(self) -> None:
        # Validate is the '\n' entry in the current mode. If no character was
        # typed we are still on the mode-choice screen: pick lower first.
        if self._kbd_mode is None:
            self._kbd_enter_mode(_MODE_LOWER)
        keys = _KEY_MAPS[self._kbd_mode]
        self._kbd_navigate_to(keys.index("\n"), ring_size=len(keys))
        self._both()
        # The keyboard page is destroyed after validation.
        self._kbd_reset_state()
