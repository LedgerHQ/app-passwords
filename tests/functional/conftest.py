# Pytest fixtures are injected by name, so parameters in fixture-consuming
# functions intentionally match module-level fixture names. Silence the noisy
# pylint warning for the whole file rather than per-function.
# pylint: disable=redefined-outer-name
import re
from pathlib import Path

import pytest
from ledgered.devices import Device
from nano.navigator import CustomNanoNavigator
from passwordsManager_cmd import PasswordsManagerCommand
from ragger.backend import BackendInterface, RaisePolicy
from ragger.navigator import Navigator
from tests_vectors import tests_vectors
from touch.navigator import CustomTouchNavigator

pytest_plugins = ("ragger.conftest.base_conftest",)


# Glue to call every test that depends on the device once for each required device
def pytest_generate_tests(metafunc):
    if "test_vector" in metafunc.fixturenames:
        metafunc.parametrize("test_vector", tests_vectors[metafunc.definition.name])


@pytest.fixture
def custom_backend(backend: BackendInterface):
    backend.raise_policy = RaisePolicy.RAISE_NOTHING
    yield backend


@pytest.fixture
def cmd(custom_backend: BackendInterface, navigator: Navigator, device: Device):
    command = PasswordsManagerCommand(custom_backend, navigator, device, True)
    yield command


@pytest.fixture
def navigator(custom_backend, device, golden_run):
    if device.is_nano:
        yield CustomNanoNavigator(custom_backend, device, golden_run)
    else:
        yield CustomTouchNavigator(custom_backend, device, golden_run)


@pytest.fixture(name="app_version")
def app_version_fixture() -> tuple[int, int, int]:
    with open(Path(__file__).parent.parent.parent / "Makefile", encoding="utf-8") as f:
        parsed = {}
        for m in re.findall(r"^APPVERSION_(\w)\s*=\s*(\d*)$", f.read(), re.MULTILINE):
            parsed[m[0]] = int(m[1])
    return (parsed["M"], parsed["N"], parsed["P"])
