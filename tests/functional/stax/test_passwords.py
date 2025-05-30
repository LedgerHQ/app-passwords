import pytest
from ragger.navigator import NavIns, NavInsID

from .navigator import CustomNavInsID


@pytest.mark.use_on_device("stax")
def test_delete_one_password(navigator, functional_test_directory):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
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
    ]
    navigator.navigate_and_compare(functional_test_directory,
                                   "delete_one_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)


@pytest.mark.use_on_device("stax")
def test_delete_all_passwords(navigator, functional_test_directory):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE_ALL,
        # confirm
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2, )),
        # check the password has been removed from the list
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(functional_test_directory,
                                   "delete_all_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)


@pytest.mark.use_on_device("stax")
def test_create_password(navigator, functional_test_directory):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
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
        # return to list to see the newly created password
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(functional_test_directory,
                                   "create_password",
                                   instructions,
                                   screen_change_before_first_instruction=False)
