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
    response = cmd.transport.exchange(cla=0xE0, ins=0x99, p1=0x01, p2=0x02, data=b"\xff\x14gmail")
    assert response.status == 0x6400  # SWO_EXECUTION_ERROR


@pytest.mark.parametrize("length", [8, 12, 16, 20, 24, 32, 48, 64])
def test_password_lengths(cmd: PasswordsManagerCommand, length: int):
    for seed in SEEDS[:10]:
        password = cmd.generate_password(0xFF, seed, version=1, length=length)
        assert len(password) == length
        for chars in GROUP_CHARS.values():
            assert any(c in chars for c in password)


@pytest.mark.parametrize("length", [0, 7, 13, 65, 255])
def test_unsupported_length_is_refused(cmd: PasswordsManagerCommand, length: int):
    response = cmd.transport.exchange(cla=0xE0, ins=0x99, p1=0x01, p2=0x01, data=bytes([0xFF, length]) + b"gmail")
    assert response.status == 0x6A80  # SWO_INCORRECT_DATA


def test_generate_password_lengths(cmd: PasswordsManagerCommand):
    for charsets, seed, length, expected in tests_vectors["test_generate_password_lengths"]:
        assert cmd.generate_password(charsets, seed, version=1, length=length) == expected


@pytest.mark.parametrize("charsets", [0x08, 0x10, 0x20, 0x38])
def test_separators_only_are_refused_for_new_entries(cmd: PasswordsManagerCommand, charsets: int):
    response = cmd.transport.exchange(cla=0xE0, ins=0x99, p1=0x01, p2=0x01, data=bytes([charsets, 20]) + b"gmail")
    assert response.status == 0x6400  # SWO_EXECUTION_ERROR


def test_separators_only_still_generate_version_0(cmd: PasswordsManagerCommand):
    # Version 0 keeps its rule: these passwords may be in use.
    assert cmd.generate_password(0x08, "gmail") == "-" * 20
    assert len(cmd.generate_password(0x38, "gmail")) == 20


@pytest.mark.parametrize("length", [4, 6])
def test_pin_lengths(cmd: PasswordsManagerCommand, length: int):
    for seed in SEEDS[:10]:
        assert cmd.generate_password(NUMBERS, seed, version=1, length=length).isdigit()


@pytest.mark.parametrize("charsets", [0x00, 0x05, 0x07, 0xFF])
def test_pin_length_needs_numbers_only(cmd: PasswordsManagerCommand, charsets: int):
    response = cmd.transport.exchange(cla=0xE0, ins=0x99, p1=0x01, p2=0x01, data=bytes([charsets, 4]) + b"bank")
    assert response.status == 0x6400  # SWO_EXECUTION_ERROR
