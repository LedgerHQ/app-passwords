from pathlib import Path

import pytest
from ragger import Firmware
from ragger.backend import SpeculosBackend, LedgerCommBackend, LedgerWalletBackend, RaisePolicy

from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import tests_vectors


APPS_DIRECTORY = Path("tests/elfs").resolve()
BACKENDS = ["speculos", "ledgercomm", "ledgerwallet"]
FIRMWARES = [
        Firmware('nanos', '2.1'),
        Firmware('nanox', '2.0.2'),
        Firmware('nanosp', '1.0.3')
]

def pytest_addoption(parser):
    parser.addoption("--backend", action="store", default="speculos")
    # Enable ussing --'device' in the pytest command line to restrict testing to specific devices
    for fw in FIRMWARES:
        parser.addoption("--"+fw.device, action="store_true", help="run on nanos only")


# Glue to call every test that depends on the firmware once for each required firmware
def pytest_generate_tests(metafunc):
    if "test_vector" in metafunc.fixturenames:
        metafunc.parametrize(
            "test_vector", tests_vectors[metafunc.definition.name])
    if "firmware" in metafunc.fixturenames:
        fw_list = []
        ids = []
        # First pass: enable only demanded firmwares
        for fw in FIRMWARES:
            if metafunc.config.getoption(fw.device):
                fw_list.append(fw)
                ids.append(fw.device + " " + fw.version)
        # Second pass if no specific firmware demanded: add them all
        if not fw_list:
            for fw in FIRMWARES:
                fw_list.append(fw)
                ids.append(fw.device + " " + fw.version)
        metafunc.parametrize("firmware", fw_list, ids=ids)

def prepare_speculos_args(firmware):
    speculos_args = []
    # Uncomment line below to enable display
    # speculos_args += ["--display", "qt"]
    application = APPS_DIRECTORY / f"passwords-{firmware.device}.elf"
    return ([application], {"args": speculos_args})


@pytest.fixture(scope="session")
def backend(pytestconfig):
    return pytestconfig.getoption("backend")


def create_backend(backend: str, firmware: Firmware):
    if backend.lower() == "ledgercomm":
        return LedgerCommBackend(firmware, interface="hid")
    elif backend.lower() == "ledgerwallet":
        return LedgerWalletBackend(firmware)
    elif backend.lower() == "speculos":
        args, kwargs = prepare_speculos_args(firmware)
        return SpeculosBackend(*args, firmware, **kwargs)
    else:
        raise ValueError(f"Backend '{backend}' is unknown. Valid backends are: {BACKENDS}")


@pytest.fixture
def client(backend, firmware):
    with create_backend(backend, firmware) as b:
        b.raise_policy = RaisePolicy.RAISE_NOTHING
        yield b


@pytest.fixture(scope="function")
def cmd(client):
    command = PasswordsManagerCommand(
        transport=client,
        debug=True
    )
    yield command

def pytest_runtest_setup(item):
    if item.config.getoption("backend") == "speculos":
        requires_phyical_device = [
            mark for mark in item.iter_markers(name="requires_phyical_device")]
        if requires_phyical_device:
            pytest.skip("test requires a real device to run")
