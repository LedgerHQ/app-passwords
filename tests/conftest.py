import pytest
import os
import subprocess
import time

from ledgercomm import Transport

from passwordsManager_cmd import PasswordsManagerCommand

from tests_vectors import tests_vectors


def pytest_addoption(parser):
    parser.addoption("--hid",
                     action="store_true")


@pytest.fixture(scope="function")
def speculos(pytestconfig):
    if pytestconfig.getoption("hid"):
        yield False
    else:
        if os.getenv("GITHUB_ACTIONS"):
            speculos_path = "/speculos/speculos.py"
        else:
            speculos_path = "speculos.py"
        with open("speculos.log", 'a') as logs_file:
            speculos_handler = subprocess.Popen([speculos_path, "bin/app.elf", "--sdk", "1.6", "--apdu-port",
                                                 "9999", "--button-port", "42000", "--automation-port", "43000", "--display", "headless", "--automation", "file:tests/automation.json"], stdout=logs_file, stderr=logs_file)
        yield True
        speculos_handler.kill()
        speculos_handler.wait()


@pytest.fixture(scope="function")
def transport(speculos):
    if speculos:
        while True:
            try:
                transport = Transport(interface="tcp", debug=True)
                break
            except ConnectionRefusedError:
                time.sleep(0.1)
        yield transport
    else:
        yield Transport(interface="hid", debug=True)


@pytest.fixture(scope="function")
def cmd(transport):
    command = PasswordsManagerCommand(
        transport=transport,
        debug=True
    )

    yield command

    command.transport.close()


@pytest.fixture(scope="function")
def cmd_(transport):
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
