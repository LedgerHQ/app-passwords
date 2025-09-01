import pytest
from ragger.backend import RaisePolicy, BackendInterface
from ragger.navigator import Navigator

from ledgered.devices import Device
from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import tests_vectors
from touch.navigator import CustomTouchNavigator

pytest_plugins = ("ragger.conftest.base_conftest", )


# Glue to call every test that depends on the device once for each required device
def pytest_generate_tests(metafunc):
    if "test_vector" in metafunc.fixturenames:
        metafunc.parametrize(
            "test_vector", tests_vectors[metafunc.definition.name])


@pytest.fixture
def custom_backend(backend):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    yield backend


@pytest.fixture
def cmd(custom_backend: BackendInterface, navigator: Navigator, device: Device):
    command = PasswordsManagerCommand(custom_backend, navigator, device, True)
    yield command


@pytest.fixture
def navigator(custom_backend, device, golden_run):
    touchNav = CustomTouchNavigator(custom_backend, device, golden_run)
    yield touchNav


@pytest.fixture(autouse=True)
def use_on_device(request, device: Device):
    if request.node.get_closest_marker('use_on_device'):
        dev_list = [d.lower() for d in request.node.get_closest_marker('use_on_device').args[0]]
        if device.name not in dev_list:
            pytest.skip(f'skipped on this device: "{device}" is not in '
                        f'{dev_list}')


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "use_on_device(device): skip test if not on the specified device",
    )
    config.addinivalue_line(
        "markers",
        "requires_physical_device(): skip test if not on a physical device"
    )
