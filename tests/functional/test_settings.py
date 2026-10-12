from pathlib import Path

from ledgered.devices import Device
from ragger.navigator import Navigator, NavIns, NavInsID
from touch.navigator import CustomNavInsID


def test_settings_screens(navigator: Navigator, device: Device, default_screenshot_path: Path):
    # Settings: the "Character types", "Password length" and "Host keyboard" entries, each opening
    # its sub-menu, then the "Press enter" switch.
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_SETTINGS,
    ]
    if device.is_nano:
        # One step per entry; "Back" leaves a sub-menu on its entry.
        instructions += [CustomNavInsID.BUTTON_APPROVE]
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 4
        instructions += [CustomNavInsID.SETTINGS_TO_HOME]
        # Password length: select 12 characters.
        instructions += [CustomNavInsID.SETTINGS_NEXT, CustomNavInsID.BUTTON_APPROVE]
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 3
        instructions += [CustomNavInsID.BUTTON_APPROVE, CustomNavInsID.SETTINGS_TO_HOME]
        # Host keyboard: select QWERTZ (DE).
        instructions += [CustomNavInsID.SETTINGS_NEXT, CustomNavInsID.BUTTON_APPROVE]
        instructions += [CustomNavInsID.SETTINGS_NEXT] * 3
        instructions += [CustomNavInsID.BUTTON_APPROVE, CustomNavInsID.SETTINGS_TO_HOME]
        # Press enter.
        instructions += [CustomNavInsID.SETTINGS_NEXT]
    else:
        # The three entries on the first page; the character types and lengths span two pages.
        instructions += [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (1,)), CustomNavInsID.SETTINGS_NEXT]
        instructions += [CustomNavInsID.SETTINGS_TO_HOME]
        instructions += [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (2,)), CustomNavInsID.SETTINGS_NEXT]
        instructions += [CustomNavInsID.SETTINGS_TO_HOME]
        instructions += [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (3,))]
        instructions += [CustomNavInsID.SETTINGS_TO_HOME]
        # Press enter.
        instructions += [CustomNavInsID.SETTINGS_NEXT]
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
        # Open "Character types", then one switch per step: uppercase, lowercase, numbers.
        toggles = [
            CustomNavInsID.BUTTON_APPROVE,
            CustomNavInsID.BUTTON_APPROVE,
            CustomNavInsID.SETTINGS_NEXT,
            CustomNavInsID.BUTTON_APPROVE,
            CustomNavInsID.SETTINGS_NEXT,
            CustomNavInsID.BUTTON_APPROVE,
        ]
    else:
        # Open "Character types": uppercase, lowercase and numbers are its first three switches.
        toggles = [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (position,)) for position in (1, 1, 2, 3)]
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
        # "Character types", then "Password length"; its sub-menu starts on 4 digits.
        open_lengths = [CustomNavInsID.SETTINGS_NEXT, CustomNavInsID.BUTTON_APPROVE]
        choose_pin = [CustomNavInsID.BUTTON_APPROVE]
    else:
        # "Password length" is the second entry of the first page; 4 digits comes first.
        open_lengths = [NavIns(CustomNavInsID.SETTINGS_CHOOSE, (2,))]
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
