from pathlib import Path

from ledgered.devices import Device
from ragger.navigator import Navigator
from touch.navigator import CustomNavInsID


def test_settings_screens(navigator: Navigator, device: Device, default_screenshot_path: Path):
    # Touch: settings span 2 pages (switches, then keyboard layout choices).
    # Nano (horizontal flow): 6 switches (uppercase, lowercase, numbers, bars,
    #  ext_symbols, no_enter) + 1 "Host keyboard" BARS_LIST entry that opens
    #  the layout sub-menu.
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_SETTINGS,
    ]
    if device.is_nano:
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 6
        instructions += [CustomNavInsID.BUTTON_APPROVE]
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 2
        instructions += [CustomNavInsID.BUTTON_APPROVE]
        instructions += [CustomNavInsID.SETTINGS_NEXT]
        instructions += [CustomNavInsID.BUTTON_APPROVE]
    else:
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 2
    instructions += [CustomNavInsID.SETTINGS_TO_HOME]

    navigator.navigate_and_compare(
        default_screenshot_path,
        "settings",
        instructions,
        screen_change_before_first_instruction=False,
    )
