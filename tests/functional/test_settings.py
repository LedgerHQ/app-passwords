from pathlib import Path

from ledgered.devices import Device
from ragger.navigator import Navigator, NavIns, NavInsID
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
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 3
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


def test_settings_keep_letters_numbers_or_specials(navigator: Navigator, device: Device, default_screenshot_path: Path):
    # Disable uppercase, lowercase, then numbers: with special characters off, the last change would
    # leave no letters, numbers or special characters, so it is refused and numbers stay enabled.
    if device.is_nano:
        # One switch per step: uppercase, lowercase, numbers.
        toggles = [
            CustomNavInsID.BUTTON_APPROVE,
            CustomNavInsID.SETTINGS_NEXT,
            CustomNavInsID.BUTTON_APPROVE,
            CustomNavInsID.SETTINGS_NEXT,
            CustomNavInsID.BUTTON_APPROVE,
        ]
    else:
        # Uppercase, lowercase and numbers are the first three switches of the first page.
        toggles = [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (position,)) for position in (1, 2, 3)]
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_SETTINGS,
        *toggles,
        # The refusal message, then back to the settings.
        NavInsID.WAIT_FOR_SCREEN_CHANGE,
    ]

    navigator.navigate_and_compare(
        default_screenshot_path,
        "settings_keep_strong_charset",
        instructions,
        screen_change_before_first_instruction=False,
    )


def test_settings_refuse_pin_length_with_letters(navigator: Navigator, device: Device, default_screenshot_path: Path):
    # With the default charsets (letters and numbers), choosing a 4-digit PIN length is refused and
    # the length sub-menu comes back.
    if device.is_nano:
        # 6 switches, "Host keyboard", then "Password length"; its sub-menu starts on 4 digits.
        open_lengths = [CustomNavInsID.SETTINGS_NEXT] * 7 + [CustomNavInsID.BUTTON_APPROVE]
        choose_pin = [CustomNavInsID.BUTTON_APPROVE]
    else:
        # Switches, keyboard layouts, then the "Password length" entry; 4 digits comes first.
        open_lengths = [CustomNavInsID.SETTINGS_NEXT] * 3 + [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (1,))]
        choose_pin = [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (1,))]
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_SETTINGS,
        *open_lengths,
        *choose_pin,
        # The refusal message, then back to the length sub-menu.
        NavInsID.WAIT_FOR_SCREEN_CHANGE,
    ]

    navigator.navigate_and_compare(
        default_screenshot_path,
        "settings_pin_length",
        instructions,
        screen_change_before_first_instruction=False,
    )
