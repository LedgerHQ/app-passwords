from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.firmware.nbgl import MetaScreen
from ragger.firmware.nbgl.use_cases import UseCaseChoice, UseCaseHomeExt, UseCaseReview, \
    UseCaseSettings
from ragger.firmware.nbgl.layouts import ChoiceList, FullKeyboardLetters, \
    LeftHeader, TappableCenter


class RadioList:

    def __init__(self, backend: BackendInterface, firmware: Firmware):
        self.backend = backend
        self.firmware = firmware

    def choose(self, index: int):
        if self.firmware == Firmware.STAX:
            positions = [(200, 130), (200, 210), (200, 290), (200, 370), (200, 450)]
        else:  # Firmware.EUROPA
            positions = [(240, 140), (240, 230), (240, 320), (240, 410), (240, 500)]
        assert 0 <= index <= 4, "Choice index must be in [0, 4]"
        self.backend.finger_touch(*positions[index])


class CustomNBGLScreen(metaclass=MetaScreen):

    # "backup your data" disclaimer, displayed the first time the app is started
    use_case_disclaimer = UseCaseChoice
    # "choose your keyboard layout" choice, displayed the first time the app is started
    layout_kbl_choice = ChoiceList
    # home page
    use_case_home = UseCaseHomeExt
    # app settings
    use_case_settings = UseCaseSettings
    # main app menu, where to choose actions (write, display, create, delete, ... passwords)
    use_case_menu = UseCaseSettings
    use_case_confirmation = UseCaseReview
    # choose a password menu (for action like write, display or delete)
    layout_menu_choice = ChoiceList
    layout_list_choice = RadioList
    # Keyboard for naming password when creating one
    layout_keyboard = FullKeyboardLetters
    # confirm and cancel action (delete, delete all, create, ...)
    layout_keyboard_confirm = TappableCenter
    layout_keyboard_cancel = LeftHeader
