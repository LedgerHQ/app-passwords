import pytest

from ragger.navigator import Navigator

from .navigator import CustomNavInsID


@pytest.mark.use_on_device(["stax", "flex", "apex_p"])
def test_immediate_quit(navigator: Navigator):
    navigator.navigate([CustomNavInsID.DISCLAIMER_CONFIRM],
                       screen_change_before_first_instruction=False)
