from ledgered.devices import Device, DeviceType
from ragger.backend import BackendInterface
from ragger.firmware.touch import MetaScreen
from ragger.firmware.touch.use_cases import UseCaseChoice, UseCaseHomeExt, UseCaseReview, \
    UseCaseSettings
from ragger.firmware.touch.layouts import ChoiceList, FullKeyboardLetters, LeftHeader

class KeyboardConfirmationButton:

    def __init__(self, backend: BackendInterface, device: Device):
        self.backend = backend
        self.device = device

    def tap(self):
        if self.device.type == DeviceType.STAX:
            position = (200, 300)
        elif self.device.type == DeviceType.FLEX:
            position = (240, 250)
        else:
            assert False, f"Device not supported: {self.device}"
        self.backend.finger_touch(*position)


class ApproveButton:

    def __init__(self, backend: BackendInterface, device: Device):
        self.backend = backend
        self.device = device

    def tap(self):
        if self.device.type == DeviceType.STAX:
            position = (200, 540)
        elif self.device.type == DeviceType.FLEX:
            position = (240, 450)
        elif self.device.type == DeviceType.APEX_P:
            position = (140, 310)
        else:
            assert False, f"Device not supported: {self.device}"
        self.backend.finger_touch(*position)


class CustomTouchScreen(metaclass=MetaScreen):

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
    layout_list_choice = ChoiceList
    # Keyboard for naming password when creating one
    layout_keyboard = FullKeyboardLetters
    # confirm and cancel action (delete, delete all, create, ...)
    layout_keyboard_confirm = KeyboardConfirmationButton
    layout_keyboard_cancel = LeftHeader
    # confirm and cancel action (delete, delete all, create, ...)
    layout_button = ApproveButton
