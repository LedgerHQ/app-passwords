import pytest
from pathlib import Path
from ragger.backend import RaisePolicy

from ledgered.devices import Device, Devices
from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import tests_vectors
from stax.navigator import CustomStaxNavigator

pytest_plugins = ("ragger.conftest.base_conftest", )


# Glue to call every test that depends on the device once for each required device
def pytest_generate_tests(metafunc):
    if "test_vector" in metafunc.fixturenames:
        metafunc.parametrize(
            "test_vector", tests_vectors[metafunc.definition.name])


@pytest.fixture(scope="session")
def functional_test_directory(root_pytest_dir) -> Path:
    return root_pytest_dir / "tests" / "functional"


@pytest.fixture
def custom_backend(backend):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    yield backend


@pytest.fixture
def cmd(custom_backend, device):
    command = PasswordsManagerCommand(
        transport=custom_backend,
        device=device,
        debug=True
    )
    yield command


@pytest.fixture
def navigator(custom_backend, device, golden_run):
    navigator = CustomStaxNavigator(custom_backend, device, golden_run)
    yield navigator


@pytest.fixture(autouse=True)
def use_on_device(request, device: Device):
    if request.node.get_closest_marker('use_on_device'):
        current_device = request.node.get_closest_marker('use_on_device').args[0].lower()
        if Devices.get_by_name(current_device).type != device.type:
            pytest.skip(f'skipped on this device: "{device}" is not '\
                        f'"{current_device}"')


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "use_on_device(device): skip test if not on the specified device",
    )
    config.addinivalue_line(
        "markers",
        "requires_physical_device(): skip test if not on a physical device"
    )
