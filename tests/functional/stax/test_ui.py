import pytest
from ragger.navigator import NavIns, NavInsID
from requests.exceptions import ConnectionError
from typing import Iterable, Union

from .navigator import CustomNavInsID


def format_instructions(instructions: Iterable[Union[NavIns, CustomNavInsID]]) -> Iterable[NavIns]:
    return [NavIns(instruction) if isinstance(instruction, CustomNavInsID) else instruction
            for instruction in instructions]


@pytest.mark.use_on_firmware("stax")
def test_immediate_quit(navigator):
    navigator.navigate(format_instructions([CustomNavInsID.HOME_TO_QUIT]),
                       screen_change_before_first_instruction=False,
                       screen_change_after_last_instruction=False)


@pytest.mark.use_on_firmware("stax")
def test_settings_screens(navigator):
    instructions = format_instructions([
        CustomNavInsID.HOME_TO_SETTINGS,
        CustomNavInsID.SETTINGS_TO_HOME,
        CustomNavInsID.HOME_TO_QUIT
    ])
    navigator.navigate(instructions,
                       screen_change_before_first_instruction=False,
                       screen_change_after_last_instruction=False)


@pytest.mark.use_on_firmware("stax")
def test_delete_one_password(navigator, functional_test_directory):
    instructions = format_instructions([
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE,
        # choose the password to delete then confirm
        NavIns(CustomNavInsID.LIST_CHOOSE, (2, )),
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        CustomNavInsID.MENU_TO_DISPLAY,
        CustomNavInsID.LIST_TO_MENU
    ])
    navigator.navigate_and_compare(functional_test_directory,
                                   "delete_one_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)


@pytest.mark.use_on_firmware("stax")
def test_delete_all_passwords(navigator, functional_test_directory):
    instructions = format_instructions([
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE_ALL,
        # confirm
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        CustomNavInsID.MENU_TO_DISPLAY,
    ])
    navigator.navigate_and_compare(functional_test_directory,
                                   "delete_all_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)


@pytest.mark.use_on_firmware("stax")
def test_create_password(navigator, functional_test_directory):
    instructions = format_instructions([
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DISPLAY,
        CustomNavInsID.LIST_TO_MENU,
        # create a new password
        CustomNavInsID.MENU_TO_CREATE,
        NavIns(CustomNavInsID.KEYBOARD_WRITE, ("n", )),
        NavIns(CustomNavInsID.KEYBOARD_WRITE, ("e", )),
        NavIns(CustomNavInsID.KEYBOARD_WRITE, ("w", )),
        CustomNavInsID.KEYBOARD_TO_CONFIRM,
        NavIns(NavInsID.WAIT, (2.5, )),
        # return to list to see the newly created password
        CustomNavInsID.MENU_TO_DISPLAY,
    ])
    navigator.navigate_and_compare(functional_test_directory,
                                   "create_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)
