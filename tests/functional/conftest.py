import pytest
from pathlib import Path
from ragger.backend import RaisePolicy

from passwordsManager_cmd import PasswordsManagerCommand
from tests_vectors import tests_vectors
from nbgl.navigator import CustomNBGLNavigator

from ragger.conftest import configuration

pytest_plugins = ("ragger.conftest.base_conftest", )


# Glue to call every test that depends on the firmware once for each required firmware
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
def cmd(custom_backend, firmware):
    command = PasswordsManagerCommand(
        transport=custom_backend,
        firmware=firmware,
        debug=True
    )
    yield command


@pytest.fixture
def navigator(custom_backend, firmware):
    navigator = CustomNBGLNavigator(custom_backend, firmware)
    yield navigator


@pytest.fixture(autouse=True)
def use_on_firmware(request, firmware):
    if request.node.get_closest_marker('use_on_firmware'):
        accepted_firmware = request.node.get_closest_marker('use_on_firmware').args[0]
        if isinstance(accepted_firmware, str):
            accepted_firmware = [accepted_firmware]

        if firmware.device not in [f.lower() for f in accepted_firmware]:
            pytest.skip(f'skipped on this firmware: "{firmware.device}" is not '\
                        f'"{current_firmware}"')


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "use_on_firmware(firmware): skip test if not on the specified firmware",
    )
    config.addinivalue_line(
        "markers",
        "requires_physical_device(): skip test if not on a physical device"
    )
