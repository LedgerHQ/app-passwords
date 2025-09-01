from pathlib import Path
import pytest

from ragger.navigator import Navigator

from .navigator import CustomNavInsID


@pytest.mark.use_on_device(["stax", "flex"])
def test_settings_screens(navigator: Navigator, default_screenshot_path: Path):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.HOME_TO_SETTINGS,
        CustomNavInsID.SETTINGS_NEXT,
        CustomNavInsID.SETTINGS_NEXT,
        CustomNavInsID.SETTINGS_TO_HOME,
    ]
    navigator.navigate_and_compare(default_screenshot_path,
                                   "settings",
                                   instructions,
                                   screen_change_before_first_instruction=False)
