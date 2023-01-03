from ragger.backend import BackendInterface
from ragger.firmware import Firmware
from ragger.firmware.fatstacks import MetaScreen
from ragger.firmware.fatstacks.use_cases import UseCaseHomeExt, UseCaseReview, UseCaseSettings
from ragger.firmware.fatstacks.layouts import ChoiceList, FullKeyboardLetters, \
    LeftHeader, TappableCenter


class RadioList:

    def __init__(self, backend: BackendInterface, firmware: Firmware):
        self.backend = backend
        self.firmware = firmware

    def choose(self, index: int):
        positions = [(200, 130), (200, 210), (200, 290), (200, 370), (200, 450)]
        assert 0 <= index <= 4, "Choice index must be in [0, 4]"
        self.backend.finger_touch(*positions[index])


class FatstacksScreen(metaclass=MetaScreen):

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
        try:
            self.home.quit()
        except:
            # when the application exits, the Speculos emulator shuts down and the Speculos client
            # throws a ConnectionError, so an error is expected here
            return
        else:
            # if no error was raised, it is likely the emulator still runs, and the application did
            # not exit
            raise RuntimeError("The application did not exit at this state")
