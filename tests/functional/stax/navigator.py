from enum import auto, Enum
from functools import partial
from ragger.navigator.navigator import Navigator
from time import sleep

from .screen import StaxScreen


class NavInsID(Enum):
    WAIT = auto()
    TOUCH = auto()
    # home screen
    HOME_TO_QUIT = auto()
    HOME_TO_SETTINGS = auto()
    HOME_TO_MENU = auto()
    # settings
    SETTINGS_TO_HOME = auto()
    SETTINGS_NEXT = auto()
    SETTINGS_PREVIOUS = auto()
    # menu
    MENU_TO_HOME = auto()
    MENU_TO_TYPE = auto()
    MENU_TO_DISPLAY = auto()
    MENU_TO_CREATE = auto()
    MENU_TO_DELETE = auto()
    MENU_TO_DELETE_ALL = auto()
    # option with a list of password (type, show, delete)
    LIST_TO_MENU = auto()
    LIST_NEXT = auto()
    LIST_PREVIOUS = auto()
    # option confirm (password deletion, all passwords deletion)
    CONFIRM_YES = auto()
    CONFIRM_NO = auto()
    # option with a keyboard (create password)
    KEYBOARD_WRITE = auto()
    KEYBOARD_TO_CONFIRM = auto()
    KEYBOARD_TO_MENU = auto()
    # choosing option in choice lists (settings, menu)
    LIST_CHOOSE = auto()

class StaxNavigator(Navigator):

    def __init__(self, backend, firmware):
        self.screen = StaxScreen(backend, firmware)
        callbacks = {
            NavInsID.WAIT: sleep,
            NavInsID.TOUCH: backend.finger_touch,
            NavInsID.HOME_TO_SETTINGS: self.screen.home.settings,
            NavInsID.HOME_TO_QUIT: self.screen.quit,
            NavInsID.HOME_TO_MENU: self.screen.home.action,
            NavInsID.SETTINGS_PREVIOUS: self.screen.settings.previous,
            NavInsID.SETTINGS_TO_HOME: self.screen.settings.multi_page_exit,
            NavInsID.SETTINGS_NEXT: self.screen.settings.next,
            NavInsID.MENU_TO_HOME: self.screen.menu.single_page_exit,
            NavInsID.MENU_TO_TYPE: partial(self.screen.menu_choice.choose, 1),
            NavInsID.MENU_TO_DISPLAY: partial(self.screen.menu_choice.choose, 2),
            NavInsID.MENU_TO_CREATE: partial(self.screen.menu_choice.choose, 3),
            NavInsID.MENU_TO_DELETE: partial(self.screen.menu_choice.choose, 4),
            NavInsID.MENU_TO_DELETE_ALL: partial(self.screen.menu_choice.choose, 5),
            NavInsID.LIST_TO_MENU: self.screen.settings.multi_page_exit,
            NavInsID.LIST_NEXT: self.screen.settings.next,
            NavInsID.LIST_PREVIOUS: self.screen.settings.previous,
            NavInsID.CONFIRM_YES: self.screen.confirmation.confirm,
            NavInsID.CONFIRM_NO: self.screen.confirmation.reject,
            NavInsID.KEYBOARD_WRITE: self._write,
            NavInsID.KEYBOARD_TO_CONFIRM: self.screen.keyboard_confirm.tap,
            NavInsID.KEYBOARD_TO_MENU: self.screen.keyboard_cancel.tap,
            NavInsID.LIST_CHOOSE: self.screen.list_choice.choose
        }
        super().__init__(backend, firmware, callbacks) #, golden_run=True)

    def _write(self, characters: str):
        # keyboard write is not an exact science on Ragger for now. The instruction together with
        # the `wait_for_screen_change` function can get messy, as the write performs multiple
        # `finger_touch` and the screen change several time, so `navigate_and_compare` could think
        # the screen can be compared, when it has not reached its last state yet.
        # Adding extra time after writing to have a better chance to get the expected screen
        self.screen.keyboard.write(characters)
        sleep(1)
