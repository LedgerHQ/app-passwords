from pathlib import Path

from ledgered.devices import Device
from ragger.navigator import Navigator, NavIns
from touch.navigator import CustomNavInsID


def test_settings_screens(navigator: Navigator, device: Device, default_screenshot_path: Path):
    # Touch: settings pages are the switches, the keyboard layout choices, then
    #  the "Password length" entry, which opens the length choices sub-menu.
    # Nano (horizontal flow): 6 switches (uppercase, lowercase, numbers, bars,
    #  ext_symbols, no_enter) + the "Host keyboard" and "Password length"
    #  BARS_LIST entries, each opening its sub-menu.
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
        instructions += [CustomNavInsID.SETTINGS_TO_HOME]
        # "Password length" BARS_LIST entry and its sub-menu: select 12 characters.
        instructions += [CustomNavInsID.SETTINGS_NEXT]
        instructions += [CustomNavInsID.BUTTON_APPROVE]
        instructions += [CustomNavInsID.SETTINGS_NEXT]
        instructions += [CustomNavInsID.BUTTON_APPROVE]
    else:
        # Switches, keyboard layouts, then the "Password length" entry and its sub-menu.
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 3
        instructions += [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (1,))]
        instructions += [CustomNavInsID.SETTINGS_NEXT]
        instructions += [CustomNavInsID.SETTINGS_TO_HOME]
    instructions += [CustomNavInsID.SETTINGS_TO_HOME]

    navigator.navigate_and_compare(
        default_screenshot_path,
        "settings",
        instructions,
        screen_change_before_first_instruction=False,
    )
