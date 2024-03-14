import pytest
from requests.exceptions import ConnectionError

from .navigator import CustomNavInsID


@pytest.mark.use_on_firmware("stax")
def test_immediate_quit(navigator):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_QUIT
    ]
    with pytest.raises(ConnectionError):
        navigator.navigate(instructions,
                           screen_change_before_first_instruction=False,
                           screen_change_after_last_instruction=False)
