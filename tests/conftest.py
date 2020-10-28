import pytest
import os
from functools import lru_cache

from ledgercomm import Transport

from passwordsManager_cmd import PasswordsManagerCommand

from tests_vectors import tests_vectors


def pytest_addoption(parser):
    parser.addoption("--hid",
                     action="store_true")


@pytest.fixture(scope="module")
def hid(pytestconfig):
    return pytestconfig.getoption("hid")


@pytest.fixture(scope="function")
def cmd(hid):
    transport = (Transport(interface="hid", debug=True)
                 if hid else Transport(interface="tcp", debug=True))
    command = PasswordsManagerCommand(
        transport=transport,
        debug=True
    )

    yield command

    command.transport.close()


def pytest_generate_tests(metafunc):
    if "test_vector" in metafunc.fixturenames:
        metafunc.parametrize(
            "test_vector", tests_vectors[metafunc.definition.name])
