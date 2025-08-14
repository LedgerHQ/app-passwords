from enum import auto
from functools import partial
from time import sleep

from ragger.navigator import NavInsID, BaseNavInsID
from ragger.navigator.navigator import Navigator

from .screen import CustomStaxScreen


class CustomNavInsID(BaseNavInsID):
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
    # Keyboard layout selection
    CHOOSE_KBL_QWERTY = auto()
    CHOOSE_KBL_QWERTY_INTL = auto()
    CHOOSE_KBL_AZERTY = auto()
    # startup disclaimer
    DISCLAIMER_CONFIRM = auto()
    DISCLAIMER_REJECT = auto()


class CustomStaxNavigator(Navigator):

    def __init__(self, backend, device, golden_run):
        self.screen = CustomStaxScreen(backend, device)
        callbacks = {
            # has to be defined for Ragger Navigator internals
            NavInsID.WAIT: sleep,
            CustomNavInsID.WAIT: sleep,
            CustomNavInsID.TOUCH: backend.finger_touch,
            CustomNavInsID.HOME_TO_SETTINGS: self.screen.home.settings,
            CustomNavInsID.HOME_TO_QUIT: self.screen.home.quit,
            CustomNavInsID.HOME_TO_MENU: self.screen.home.action,
            CustomNavInsID.SETTINGS_PREVIOUS: self.screen.settings.previous,
            CustomNavInsID.SETTINGS_TO_HOME: self.screen.settings.multi_page_exit,
            CustomNavInsID.SETTINGS_NEXT: self.screen.settings.next,
            CustomNavInsID.MENU_TO_HOME: self.screen.menu.single_page_exit,
            CustomNavInsID.MENU_TO_TYPE: partial(self.screen.menu_choice.choose, 1),
            CustomNavInsID.MENU_TO_DISPLAY: partial(self.screen.menu_choice.choose, 2),
            CustomNavInsID.MENU_TO_CREATE: partial(self.screen.menu_choice.choose, 3),
            CustomNavInsID.MENU_TO_DELETE: partial(self.screen.menu_choice.choose, 4),
            CustomNavInsID.MENU_TO_DELETE_ALL: partial(self.screen.menu_choice.choose, 5),
            CustomNavInsID.LIST_TO_MENU: self.screen.settings.multi_page_exit,
            CustomNavInsID.LIST_NEXT: self.screen.settings.next,
            CustomNavInsID.LIST_PREVIOUS: self.screen.settings.previous,
            CustomNavInsID.CONFIRM_YES: self.screen.confirmation.confirm,
            CustomNavInsID.CONFIRM_NO: self.screen.confirmation.reject,
            CustomNavInsID.KEYBOARD_WRITE: self.screen.keyboard.write,
            CustomNavInsID.KEYBOARD_TO_CONFIRM: self.screen.keyboard_confirm.tap,
            CustomNavInsID.KEYBOARD_TO_MENU: self.screen.keyboard_cancel.tap,
            CustomNavInsID.LIST_CHOOSE: self._choose,
            CustomNavInsID.CHOOSE_KBL_QWERTY: partial(self._choose, 0),
            CustomNavInsID.CHOOSE_KBL_QWERTY_INTL: partial(self._choose, 1),
            CustomNavInsID.CHOOSE_KBL_AZERTY: partial(self._choose, 2),
            CustomNavInsID.DISCLAIMER_CONFIRM: self.screen.disclaimer.confirm,
            CustomNavInsID.DISCLAIMER_REJECT: self.screen.disclaimer.reject,
        }
        super().__init__(backend, device, callbacks, golden_run)

    def _choose(self, position: int):
        # Choosing a field in settings list will display a temporary screen where the chosen field
        # is highlighted, then will go the result page. The sleep helps **not** catching this
        # intermediate screen
        self.screen.list_choice.choose(position)
        sleep(0.2)
