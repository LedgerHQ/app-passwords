from pathlib import Path
from time import sleep

import pytest
from passwordsManager_cmd import PasswordsManagerCommand
from ragger.backend import BackendInterface
from ragger.navigator import Navigator, NavIns, NavInsID
from touch.navigator import CustomNavInsID

# SETS byte and entry layout reused from EXISTING_METADATA in tests_vectors.py:
# [DATALEN=0x0a][KIND=0x00][SETS=0x07][9-char nickname]. The SETS byte also
# selects the charsets used by the password derivation, so the expected value
# of an entry can be recomputed independently with generate_password(SETS, name).
_SETS = 0x07


def _metadata_entry(name: str) -> bytes:
    assert len(name) == 9, "nickname must be 9 chars to match the fixed DATALEN"
    return bytes([0x0A, 0x00, _SETS]) + name.encode()


def _screen_texts(backend: BackendInterface) -> list:
    return [event["text"] for event in backend.get_current_screen_content()["events"]]


def test_delete_one_password(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE,
        # choose the password to delete then confirm
        NavIns(CustomNavInsID.LIST_CHOOSE, (2,)),
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2,)),
        # check the password has been removed from the list
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(
        default_screenshot_path,
        "delete_one_password",
        instructions,
        screen_change_before_first_instruction=False,
    )


def test_delete_all_passwords(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DELETE_ALL,
        # confirm
        CustomNavInsID.CONFIRM_YES,
        NavIns(NavInsID.WAIT, (2,)),
    ]
    navigator.navigate_and_compare(
        default_screenshot_path,
        "delete_all_password",
        instructions,
        screen_change_before_first_instruction=False,
    )


def test_create_password(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_MENU,
        # ensure the password list is filled with populated passwords
        CustomNavInsID.MENU_TO_DISPLAY,
        CustomNavInsID.LIST_TO_MENU,
        # create a new password
        CustomNavInsID.MENU_TO_CREATE,
        NavIns(CustomNavInsID.KEYBOARD_WRITE, ("n",)),
        NavIns(CustomNavInsID.KEYBOARD_WRITE, ("e",)),
        NavIns(CustomNavInsID.KEYBOARD_WRITE, ("w",)),
        CustomNavInsID.KEYBOARD_TO_CONFIRM,
        NavInsID.WAIT_FOR_SCREEN_CHANGE,
        # return to list to see the newly created password
        CustomNavInsID.MENU_TO_DISPLAY,
    ]
    navigator.navigate_and_compare(
        default_screenshot_path,
        "create_password",
        instructions,
        screen_change_before_first_instruction=False,
    )


@pytest.mark.skip_nano  # Nano lists one choice per page; this targets the touch pagination
def test_show_password_on_second_page(
    cmd: PasswordsManagerCommand,
    navigator: Navigator,
    custom_backend: BackendInterface,
    default_screenshot_path: Path,
):
    """Showing a password located on page >= 2 of the list must reveal that
    exact entry's value.

    On wallet devices the choices list is paginated and NBGL forwards a
    page-relative index, so password_callback() rebuilds the absolute index
    from the page number (page * nbPasswordsPerPage + index). This test guards
    against an off-by-page regression in that recalculation: it seeds enough
    passwords to span at least two pages, selects the first entry of page 2,
    and asserts the shown value matches the value derived independently for
    that nickname.
    """
    # Seed enough 9-char nicknames to guarantee more than one page on every
    # wallet screen size (Flex shows 4 per page, larger screens a few more).
    names = [f"pwd{i:06d}" for i in range(12)]
    metadata = b"".join(_metadata_entry(name) for name in names)

    navigator.navigate(
        [
            CustomNavInsID.DISCLAIMER_CONFIRM,
            CustomNavInsID.CHOOSE_KBL_QWERTY,
        ],
        screen_change_before_first_instruction=False,
    )
    cmd.load_metadatas(metadata)

    navigator.navigate(
        [
            CustomNavInsID.HOME_TO_MENU,
            CustomNavInsID.MENU_TO_DISPLAY,
        ],
        screen_change_before_first_instruction=False,
    )
    sleep(0.5)

    # Derive the per-page count from page 1 (the title is not a list entry).
    page1 = [t for t in _screen_texts(custom_backend) if t in names]
    nb_per_page = len(page1)
    assert nb_per_page >= 1, "no password entry rendered on the first page"
    assert nb_per_page < len(names), "all passwords fit on a single page; seed more"
    assert page1 == names[:nb_per_page], "first page must list the entries in order"

    # The first (top) entry of page 2 is the absolute index `nb_per_page`.
    expected_name = names[nb_per_page]
    expected_value = cmd.generate_password(_SETS, expected_name)

    navigator.navigate([CustomNavInsID.SETTINGS_NEXT], screen_change_before_first_instruction=False)
    sleep(0.5)
    assert expected_name in _screen_texts(custom_backend), "page 2 must start with the entry following the last entry of page 1"

    navigator.navigate_and_compare(
        default_screenshot_path,
        "pwd_on_second_page",
        [NavIns(CustomNavInsID.LIST_CHOOSE, (1,))],
        screen_change_before_first_instruction=False,
    )
    sleep(1.0)

    # The "Your Password" screen shows the nickname and its derived value; both
    # must correspond to the page-2 entry, proving the absolute index is right.
    shown = _screen_texts(custom_backend)
    assert expected_name in shown
    assert expected_value in shown, f"expected value of '{expected_name}' not shown; wrong entry selected"
