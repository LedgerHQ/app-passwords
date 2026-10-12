# CustomTouchScreen builds its members (home, settings, button, ...) through the
# ragger MetaScreen metaclass, so pylint cannot see them statically and reports
# false no-member errors on every self.screen.* access.
# pylint: disable=no-member
from enum import auto
from functools import partial
from pathlib import Path
from tempfile import TemporaryDirectory
from time import sleep, time

from ragger.navigator import BaseNavInsID, NavInsID
from ragger.navigator.navigator import Navigator

from .screen import CustomTouchScreen

# Timeout while waiting for a chosen entry to open its page.
SCREEN_SETTLE_TIMEOUT = 10.0


class CustomNavInsID(BaseNavInsID):
    # home screen
    HOME_TO_SETTINGS = auto()
    HOME_TO_MENU = auto()
    # settings
    SETTINGS_TO_HOME = auto()
    SETTINGS_NEXT = auto()
    # menu
    MENU_TO_HOME = auto()
    MENU_TO_TYPE = auto()
    MENU_TO_DISPLAY = auto()
    MENU_TO_CREATE = auto()
    MENU_TO_DELETE = auto()
    MENU_TO_DELETE_ALL = auto()
    # option with a list of password (type, show, delete)
    LIST_TO_MENU = auto()
    # option confirm (password deletion, all passwords deletion)
    CONFIRM_YES = auto()
    KEYBOARD_WRITE = auto()
    KEYBOARD_TO_CONFIRM = auto()
    # choosing option in choice lists (settings, menu)
    LIST_CHOOSE = auto()
    # choosing an option in a settings choice list, which stays on the same page
    SETTINGS_CHOOSE = auto()
    # Keyboard layout selection
    CHOOSE_KBL_QWERTY = auto()
    # startup disclaimer
    DISCLAIMER_CONFIRM = auto()
    # Generic "select/validate" tap (settings switches, BARS_LIST entries, ...)
    BUTTON_APPROVE = auto()
    # Approve an APDU confirmation prompt (nbgl_useCaseChoice)
    APDU_APPROVE = auto()
    # Refuse an APDU confirmation prompt (nbgl_useCaseChoice)
    APDU_REJECT = auto()


class CustomTouchNavigator(Navigator):
    def __init__(self, backend, device, golden_run):
        self.screen = CustomTouchScreen(backend, device)
        callbacks = {
            # has to be defined for Ragger Navigator internals
            NavInsID.WAIT: sleep,
            NavInsID.WAIT_FOR_SCREEN_CHANGE: backend.wait_for_screen_change,
            CustomNavInsID.HOME_TO_SETTINGS: self.screen.home.settings,
            CustomNavInsID.HOME_TO_MENU: self.screen.home.action,
            CustomNavInsID.SETTINGS_TO_HOME: self.screen.settings.multi_page_exit,
            CustomNavInsID.SETTINGS_NEXT: self.screen.settings.next,
            CustomNavInsID.MENU_TO_CREATE: partial(self.screen.menu_choice.choose, 1),
            CustomNavInsID.MENU_TO_TYPE: partial(self.screen.menu_choice.choose, 2),
            CustomNavInsID.MENU_TO_DISPLAY: partial(self.screen.menu_choice.choose, 3),
            CustomNavInsID.MENU_TO_DELETE: partial(self.screen.menu_choice.choose, 4),
            CustomNavInsID.MENU_TO_DELETE_ALL: partial(self.screen.menu_choice.choose, 5),
            CustomNavInsID.LIST_TO_MENU: self.screen.settings.multi_page_exit,
            CustomNavInsID.CONFIRM_YES: self.screen.confirmation.confirm,
            CustomNavInsID.KEYBOARD_WRITE: self.screen.keyboard.write,
            CustomNavInsID.KEYBOARD_TO_CONFIRM: self.screen.keyboard_confirm.tap,
            CustomNavInsID.LIST_CHOOSE: self._choose,
            CustomNavInsID.SETTINGS_CHOOSE: self.screen.list_choice.choose,
            CustomNavInsID.CHOOSE_KBL_QWERTY: partial(self._choose, 1),
            CustomNavInsID.DISCLAIMER_CONFIRM: self.screen.disclaimer.confirm,
            CustomNavInsID.BUTTON_APPROVE: self.screen.button.tap,
            CustomNavInsID.APDU_APPROVE: self.screen.button.tap,
            # The "Refuse" footer of the nbgl_useCaseChoice approval screen
            # (same layout as the disclaimer choice).
            CustomNavInsID.APDU_REJECT: self.screen.disclaimer.reject,
        }
        super().__init__(backend, device, callbacks, golden_run)

    def _choose(self, position: int):
        # Choosing an entry redraws the list with that entry checked, then the next page, which
        # can take a while to come (password derivation). The checked list shows the same text as
        # before the tap, so wait for the text to change, then for the screen to settle. The
        # ticker is paused during navigation, so each poll sends a tick to let the app run.
        texts_before = self._screen_texts()
        self.screen.list_choice.choose(position)
        deadline = time() + SCREEN_SETTLE_TIMEOUT
        while self._screen_texts() == texts_before:
            if time() > deadline:
                # The navigator then reports the missing screen change.
                return
            self._backend.send_tick()
        self._wait_for_stable_screen(deadline)

    def _screen_texts(self) -> list:
        return [event["text"] for event in self._backend.get_current_screen_content()["events"]]

    def _wait_for_stable_screen(self, deadline: float):
        """Wait until the screen is unchanged across one tick."""
        with TemporaryDirectory() as tmp_dir:
            current = Path(tmp_dir) / "current.png"
            self._backend.compare_screen_with_snapshot(current, golden_run=True)
            while time() <= deadline:
                self._backend.send_tick()
                if self._backend.compare_screen_with_snapshot(current):
                    return
                self._backend.compare_screen_with_snapshot(current, golden_run=True)
