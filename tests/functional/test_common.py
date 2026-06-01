from ragger.navigator import Navigator

from touch.navigator import CustomNavInsID


def test_immediate_quit(navigator: Navigator):
    instructions = [
        CustomNavInsID.DISCLAIMER_CONFIRM,
        CustomNavInsID.CHOOSE_KBL_QWERTY,
    ]
    navigator.navigate(instructions, screen_change_before_first_instruction=False)
