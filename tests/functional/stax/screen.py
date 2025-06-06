from ledgered.devices import Device
from ragger.backend import BackendInterface
from ragger.firmware.touch import MetaScreen
from ragger.firmware.touch.use_cases import UseCaseChoice, UseCaseHomeExt, UseCaseReview, \
    UseCaseSettings
from ragger.firmware.touch.layouts import ChoiceList, FullKeyboardLetters, \
    LeftHeader, TappableCenter


class RadioList:

    def __init__(self, backend: BackendInterface, device: Device):
        self.backend = backend
        self.device = device

    def choose(self, index: int):
        positions = [(200, 130), (200, 210), (200, 290), (200, 370), (200, 450)]
        assert 0 <= index <= 4, "Choice index must be in [0, 4]"
        self.backend.finger_touch(*positions[index])


class CustomStaxScreen(metaclass=MetaScreen):

    use_case_home = UseCaseHomeExt
    layout_kbl_choice = ChoiceList
    use_case_settings = UseCaseSettings
    use_case_menu = UseCaseSettings
    use_case_confirmation = UseCaseReview
    use_case_disclaimer = UseCaseChoice
    layout_menu_choice = ChoiceList
    layout_list_choice = RadioList
    layout_keyboard = FullKeyboardLetters
    layout_keyboard_confirm = TappableCenter
    layout_keyboard_cancel = LeftHeader
