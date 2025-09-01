from enum import auto
from functools import partial
from time import sleep

from ragger.navigator import NavInsID, BaseNavInsID
from ragger.navigator.navigator import Navigator

from .screen import CustomTouchScreen


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
    # startup disclaimer
    DISCLAIMER_CONFIRM = auto()
    # Approve metadatas
    BUTTON_APPROVE = auto()


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
            CustomNavInsID.MENU_TO_DISPLAY: partial(self.screen.menu_choice.choose, 2),
            CustomNavInsID.MENU_TO_CREATE: partial(self.screen.menu_choice.choose, 3),
            CustomNavInsID.MENU_TO_DELETE: partial(self.screen.menu_choice.choose, 4),
            CustomNavInsID.MENU_TO_DELETE_ALL: partial(self.screen.menu_choice.choose, 5),
            CustomNavInsID.LIST_TO_MENU: self.screen.settings.multi_page_exit,
            CustomNavInsID.CONFIRM_YES: self.screen.confirmation.confirm,
            CustomNavInsID.KEYBOARD_WRITE: self.screen.keyboard.write,
            CustomNavInsID.KEYBOARD_TO_CONFIRM: self.screen.keyboard_confirm.tap,
            CustomNavInsID.LIST_CHOOSE: self._choose,
            CustomNavInsID.DISCLAIMER_CONFIRM: self.screen.disclaimer.confirm,
            CustomNavInsID.BUTTON_APPROVE: self.screen.button.tap,
        }
        super().__init__(backend, device, callbacks, golden_run)

    def _choose(self, position: int):
        # Choosing a field in settings list will display a temporary screen where the chosen field
        # is highlighted, then will go the result page. The sleep helps **not** catching this
        # intermediate screen
        self.screen.list_choice.choose(position)
        sleep(0.2)
