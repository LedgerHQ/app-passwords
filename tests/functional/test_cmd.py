import pytest
from exception import MetadatasParsingError
from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import EXISTING_METADATA

STORAGE_SIZE = 4096


def test_app_info(cmd: PasswordsManagerCommand, app_version: tuple[int, int, int]):
    vers_str = ".".join(map(str, app_version))
    assert cmd.get_app_info() == ("Passwords", vers_str)


def test_app_config(cmd: PasswordsManagerCommand):
    assert cmd.get_app_config() == (4096, 0, 0)


def test_generate_password(cmd: PasswordsManagerCommand, test_vector):
    charset, seed, expected = test_vector
    assert cmd.generate_password(charset, seed) == expected


def test_dump_metadatas(cmd: PasswordsManagerCommand, test_vector):
    size, expected = test_vector
    assert cmd.dump_metadatas(size) == expected
    cmd.reset_approval_state()


def test_load_metadatas(cmd: PasswordsManagerCommand, test_vector):
    # [0] to avoid huge test names filled with the data.
    # Instead, it is filled with the data index
    metadatas = test_vector[0]
    cmd.load_metadatas(metadatas)
    assert cmd.dump_metadatas(len(metadatas)) == metadatas
    cmd.reset_approval_state()


def test_load_metadatas_interrupted_transfer_clears_database(cmd: PasswordsManagerCommand):
    # A restore that never sends its last chunk used to leave the live database as a mix of
    # the old and the new image, with metadata_count still describing the old one.
    populated = EXISTING_METADATA + b"\x00" * (STORAGE_SIZE - len(EXISTING_METADATA))
    assert cmd.dump_metadatas(STORAGE_SIZE) == populated
    cmd.reset_approval_state()

    # Approve a restore, send a single non-final chunk, then switch to another command.
    cmd.approved = False
    cmd.load_metadatas_chunk(b"\x02\x00\x07z" + b"\xaa" * 100, is_last=False)
    cmd.get_app_config()

    # Neither the old entries nor the partially written new image may survive.
    assert cmd.dump_metadatas(STORAGE_SIZE) == b"\x00" * STORAGE_SIZE
    cmd.reset_approval_state()


def test_load_metadatas_malformed_image_clears_database(cmd: PasswordsManagerCommand):
    # The parser refuses this image (a record claiming more payload than a nickname holds).
    # The refused bytes must not be left in storage for a later flow to parse.
    malformed = bytes.fromhex("150007" + "61" * 21) + b"\x00" * (STORAGE_SIZE - 24)
    with pytest.raises(MetadatasParsingError):
        cmd.load_metadatas(malformed)

    assert cmd.dump_metadatas(STORAGE_SIZE) == b"\x00" * STORAGE_SIZE
    cmd.reset_approval_state()
