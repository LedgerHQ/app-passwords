import pytest
from ragger.navigator import NavIns
from typing import Iterable, Union

from .navigator import NavInsID


def format_instructions(instructions: Iterable[Union[NavIns, NavInsID]]) -> Iterable[NavIns]:
    return [NavIns(instruction) if isinstance(instruction, NavInsID) else instruction
            for instruction in instructions]


@pytest.mark.use_on_firmware("fat")
def test_immediate_quit(navigator):
    instruction = format_instructions([NavInsID.HOME_TO_QUIT])
    navigator.navigate(instruction)


@pytest.mark.use_on_firmware("fat")
def test_settings_screens(navigator):
    instructions = format_instructions([
        NavInsID.HOME_TO_SETTINGS,
        NavInsID.SETTINGS_TO_HOME,
        NavInsID.HOME_TO_QUIT
    ])
    navigator.navigate(instructions)


@pytest.mark.use_on_firmware("fat")
def test_delete_one_password(navigator, functional_test_directory):
    instructions = format_instructions([
        NavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        NavInsID.MENU_TO_DELETE,
        # choose the password to delete then confirm
        NavIns(NavInsID.LIST_CHOOSE, (2, )),
        NavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        NavInsID.MENU_TO_DISPLAY,
        NavInsID.LIST_TO_MENU
    ])
    navigator.navigate_and_compare(functional_test_directory,
                                   "delete_one_password",
                                   instructions,
                                   screen_change_before_first_instruction=False,
                                   screen_change_after_last_instruction=False)


@pytest.mark.use_on_firmware("fat")
def test_delete_all_password(navigator, functional_test_directory):
    instructions = format_instructions([
        NavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        NavInsID.MENU_TO_DELETE_ALL,
        # confirm
        NavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        NavInsID.MENU_TO_DISPLAY,
        NavInsID.LIST_TO_MENU
    ])
    navigator.navigate_and_compare(functional_test_directory,
                                   "delete_all_password",
                                   instructions,
                                   screen_change_before_first_instruction=False,
                                   screen_change_after_last_instruction=False)


@pytest.mark.use_on_firmware("fat")
def test_create_password(navigator, functional_test_directory):
    instructions = format_instructions([
        NavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        NavInsID.MENU_TO_DISPLAY,
        NavInsID.LIST_TO_MENU,
        # create a new password
        NavInsID.MENU_TO_CREATE,
        NavIns(NavInsID.KEYBOARD_WRITE, ("new", )),
        NavInsID.KEYBOARD_TO_CONFIRM,
        NavIns(NavInsID.WAIT, (2.5, )),
        # return to list to see the newly created password
        NavInsID.MENU_TO_DISPLAY,
        NavInsID.LIST_TO_MENU
    ])
    navigator.navigate_and_compare(functional_test_directory,
                                   "create_password",
                                   instructions,
                                   screen_change_before_first_instruction=False,
                                   screen_change_after_last_instruction=False)
