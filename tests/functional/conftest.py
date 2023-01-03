from pathlib import Path

import pytest
from ragger.firmware import Firmware
from ragger.backend import SpeculosBackend, LedgerCommBackend, LedgerWalletBackend, RaisePolicy

from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import tests_vectors
from ui.fatstacks_navigator import FatstacksNavigator

FUNCTIONAL_TESTS_DIR = Path("tests/functional/").resolve()
APPS_DIRECTORY = FUNCTIONAL_TESTS_DIR / "elfs"
BACKENDS = ["speculos", "ledgercomm", "ledgerwallet"]
FIRMWARES = [
        Firmware('nanos', '2.1'),
        Firmware('nanox', '2.0.2'),
        Firmware('nanosp', '1.0.3'),
        Firmware('fat', '1.0')
]


def pytest_addoption(parser):
    parser.addoption("--backend", action="store", default="speculos")
    parser.addoption("--display", action="store_true", default=False)
    parser.addoption("--golden_run", action="store_true", default=False)
    # Enable using --'device' in the pytest command line to restrict testing to specific devices
    for fw in FIRMWARES:
        parser.addoption("--"+fw.device, action="store_true", help="run on nanos only")


@pytest.fixture(scope="session")
def backend_name(pytestconfig):
    return pytestconfig.getoption("backend")


@pytest.fixture(scope="session")
def display(pytestconfig):
    return pytestconfig.getoption("display")


@pytest.fixture(scope="session")
def golden_run(pytestconfig):
    return pytestconfig.getoption("golden_run")


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

def prepare_speculos_args(firmware: Firmware, display: bool):
    speculos_args = []
    if display:
        speculos_args += ["--display", "qt"]
    application = APPS_DIRECTORY / f"passwords-{firmware.device}.elf"
    return ([application], {"args": speculos_args})


def create_backend(backend_name: str, firmware: Firmware, display: bool):
    if backend_name.lower() == "ledgercomm":
        return LedgerCommBackend(firmware, interface="hid")
    elif backend_name.lower() == "ledgerwallet":
        return LedgerWalletBackend(firmware)
    elif backend_name.lower() == "speculos":
        args, kwargs = prepare_speculos_args(firmware, display)
        return SpeculosBackend(*args, firmware, **kwargs)
    else:
        raise ValueError(f"Backend '{backend}' is unknown. Valid backends are: {BACKENDS}")


@pytest.fixture
def backend(backend_name: str, firmware: Firmware, display: bool):
    with create_backend(backend_name, firmware, display) as b:
        b.raise_policy = RaisePolicy.RAISE_NOTHING
        yield b


@pytest.fixture(scope="function")
def cmd(backend):
    command = PasswordsManagerCommand(
        transport=backend,
        debug=True
    )
    yield command


@pytest.fixture(scope="function")
def navigator(backend, firmware):
    navigator = FatstacksNavigator(backend, firmware)
    yield navigator


@pytest.fixture(scope="function")
def screen(navigator):
    yield navigator.screen


@pytest.fixture(scope="module")
def functional_test_directory():
    yield FUNCTIONAL_TESTS_DIR


@pytest.fixture(autouse=True)
def use_on_firmware(request, firmware):
    if request.node.get_closest_marker('use_on_firmware'):
        current_firmware = request.node.get_closest_marker('use_on_firmware').args[0].lower()
        if current_firmware != firmware.device:
            pytest.skip(f'skipped on this firmware: "{firmware.device}" is not '\
                        f'"{current_firmware}"')


def pytest_configure(config):
    config.addinivalue_line(
        "markers", "use_on_firmware(firmware): skip test if not on the specified firmware",
    )


def pytest_runtest_setup(item):
    if item.config.getoption("backend") == "speculos":
        requires_phyical_device = [
            mark for mark in item.iter_markers(name="requires_phyical_device")]
        if requires_phyical_device:
            pytest.skip("test requires a real device to run")
