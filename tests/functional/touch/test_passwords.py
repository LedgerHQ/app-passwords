from pathlib import Path
import pytest

from ragger.navigator import Navigator, NavIns, NavInsID

from .navigator import CustomNavInsID


@pytest.mark.use_on_device(["stax", "flex",])
def test_delete_one_password(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE,
        # choose the password to delete then confirm
        NavIns(CustomNavInsID.LIST_CHOOSE, (2, )),
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(default_screenshot_path,
                                   "delete_one_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)


@pytest.mark.use_on_device(["stax", "flex"])
def test_delete_all_passwords(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE_ALL,
        # confirm
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(default_screenshot_path,
                                   "delete_all_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)


@pytest.mark.use_on_device(["stax", "flex"])
def test_create_password(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
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
        NavInsID.WAIT_FOR_SCREEN_CHANGE,
        # return to list to see the newly created password
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(default_screenshot_path,
                                   "create_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)
