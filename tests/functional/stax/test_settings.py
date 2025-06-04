import pytest
from requests.exceptions import ConnectionError

from .navigator import CustomNavInsID



@pytest.mark.use_on_device("stax")
def test_settings_screens(navigator, functional_test_directory):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
        CustomNavInsID.HOME_TO_SETTINGS,
        CustomNavInsID.SETTINGS_NEXT,
        CustomNavInsID.SETTINGS_NEXT,
        CustomNavInsID.SETTINGS_NEXT,
        CustomNavInsID.SETTINGS_TO_HOME,
        CustomNavInsID.HOME_TO_QUIT
    ]
    with pytest.raises(ConnectionError):
        navigator.navigate_and_compare(functional_test_directory,
                                       "settings",
                                       instructions,
                                       screen_change_before_first_instruction=False,
                                       screen_change_after_last_instruction=False)
