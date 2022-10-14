import pytest
import os
import subprocess
import time

from ledgercomm import Transport
from speculos.client import SpeculosClient

from passwordsManager_cmd import PasswordsManagerCommand

from tests_vectors import tests_vectors


APP_NANOS = "bin/passwords-app-nanos"


def pytest_addoption(parser):
    parser.addoption("--hid",
                     action="store_true")
    parser.addoption("--external-speculos-instance",
                     action="store_true")


@pytest.fixture(scope="session")
def speculos_logfile(pytestconfig):
    if pytestconfig.getoption("hid"):
        yield None
    elif pytestconfig.getoption("external_speculos_instance"):
        yield None
    else:
        yield open("speculos.log", 'w')


@pytest.fixture(scope="function")
def speculos(pytestconfig, speculos_logfile):
    if pytestconfig.getoption("hid"):
        yield False
    elif pytestconfig.getoption("external_speculos_instance"):
        yield True
    else:
        with SpeculosClient(APP_NANOS):
            yield True

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

def pytest_generate_tests(metafunc):
    if "test_vector" in metafunc.fixturenames:
        metafunc.parametrize(
            "test_vector", tests_vectors[metafunc.definition.name])


def pytest_runtest_setup(item):
    if not item.config.getoption("hid"):
        requires_phyical_device = [
            mark for mark in item.iter_markers(name="requires_phyical_device")]
        if requires_phyical_device:
            pytest.skip("test requires a real device to run")
