from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.firmware.stax import MetaScreen
from ragger.firmware.stax.use_cases import UseCaseHomeExt, UseCaseReview, UseCaseSettings
from ragger.firmware.stax.layouts import ChoiceList, FullKeyboardLetters, \
    LeftHeader, TappableCenter


class RadioList:

    def __init__(self, backend: BackendInterface, firmware: Firmware):
        self.backend = backend
        self.firmware = firmware

    def choose(self, index: int):
        positions = [(200, 130), (200, 210), (200, 290), (200, 370), (200, 450)]
        assert 0 <= index <= 4, "Choice index must be in [0, 4]"
        self.backend.finger_touch(*positions[index])


class StaxScreen(metaclass=MetaScreen):

    use_case_home = UseCaseHomeExt
    use_case_settings = UseCaseSettings
    use_case_menu = UseCaseSettings
    use_case_confirmation = UseCaseReview
    layout_menu_choice = ChoiceList
    layout_list_choice = RadioList
    layout_keyboard = FullKeyboardLetters
    layout_keyboard_confirm = TappableCenter
    layout_keyboard_cancel = LeftHeader

    def quit(self):
        self.home.quit()
