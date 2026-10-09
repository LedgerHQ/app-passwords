import pytest
from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import tests_vectors

UPPERCASE = 0x01
LOWERCASE = 0x02
NUMBERS = 0x04
SEPARATORS = 0x08 | 0x10 | 0x20
SPECIALS = 0x40 | 0x80

GROUP_CHARS = {
    UPPERCASE: "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
    LOWERCASE: "abcdefghijklmnopqrstuvwxyz",
    NUMBERS: "0123456789",
    SEPARATORS: "-_ ",
    SPECIALS: "\"#$%&'*+,./:;=?!@\\^`|~[]{}()<>",
}

SEEDS = [f"seed{i:03d}" for i in range(40)]


@pytest.mark.parametrize("charsets", [0x0F, 0x3F, 0x47, 0x7F, 0xC7, 0xFF])
def test_v1_draws_from_every_enabled_group(cmd: PasswordsManagerCommand, charsets: int):
    for seed in SEEDS:
        password = cmd.generate_password(charsets, seed, version=1)
        for group, chars in GROUP_CHARS.items():
            if charsets & group:
                assert any(c in chars for c in password), f"{seed}: no character of group {group:#x} in {password!r}"


def test_v0_only_guarantees_a_space_among_separators(cmd: PasswordsManagerCommand):
    # Version 0 keeps its rule: some passwords have separators but no '-' nor '_'.
    passwords = [cmd.generate_password(0x3F, seed) for seed in SEEDS]
    assert all(" " in p for p in passwords)
    assert any(("-" not in p) and ("_" not in p) for p in passwords)


def test_generate_password_v1(cmd: PasswordsManagerCommand):
    for charsets, seed, expected in tests_vectors["test_generate_password_v1"]:
        assert cmd.generate_password(charsets, seed, version=1) == expected


def test_unknown_version_is_refused(cmd: PasswordsManagerCommand):
    response = cmd.transport.exchange(cla=0xE0, ins=0x99, p1=0x01, p2=0x02, data=b"\xffgmail")
    assert response.status == 0x6400  # SWO_EXECUTION_ERROR
