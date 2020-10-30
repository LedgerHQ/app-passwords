import pytest


def test_version(cmd):
    assert cmd.get_version() == "0.1.0"


def test_app_name(cmd):
    assert cmd.get_app_name() == "Passwords"


def test_generate_password(cmd, test_vector):
    charset, seed, expected = test_vector
    assert cmd.generate_password(charset, seed) == expected


def test_dump_metadatas(cmd, test_vector):
    size, expected = test_vector
    assert cmd.dump_metadatas(size) == expected


def test_load_metadatas(cmd, test_vector):
    metadatas = test_vector
    cmd.load_metadatas(metadatas)
    cmd.dump_metadatas(len(metadatas))
