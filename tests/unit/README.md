# Unit tests

The unit tests use the SDK unit-test framework (`LedgerUT`), based on
[Unity](https://github.com/ThrowTheSwitch/Unity) and [CMock](https://github.com/ThrowTheSwitch/CMock).
It is documented in the SDK, in `cmake/USAGE.md`.

## Prerequisites

- CMake >= 3.10
- Ruby, used by CMock to generate the mocks
- Network access on the first configuration, to fetch Unity and CMock
- lcov >= 1.14, for the code coverage

All of them are available in the `ledger-app-builder` Docker images.
`BOLOS_SDK` must point to the SDK, as it does in these images.

## Layout

| Path | Content |
| --- | --- |
| `CMakeLists.txt` | Test declarations, one `ledger_unit_tests_add_test()` per test executable |
| `test_<name>.c` | Test executable `test_<name>` |
| `fakes/` | Hand-written replacements of SDK functions (`nvm_write`, `is_printable_string`, the `TRY`/`THROW` context), of the app globals, and of the generated `glyphs.h` |

Prefer the real SDK headers and CMock mocks (`MOCK_HEADERS`) to new fakes.

The password generation tests use the DRBG seeds the device derives from the Speculos default seed,
so their expected passwords are the ones of the functional tests.
The persistent storage `N_storage_real` is defined writable in `fakes/fake_vars.c`:
tests access it through `N_storage_rw`, from `fakes/fake_vars.h`.

## Build and run

In `tests/unit` folder:

```shell
cmake -B build -S .
cmake --build build
ctest --test-dir build --output-on-failure
```

A single test executable can also be run directly, for example `build/test_password`.

## Code coverage

After running the tests:

```shell
cmake --build build --target generate_coverage
```

It outputs `build/coverage.info`, `build/coverage.xml`, and an HTML report in `build/coverage/index.html`.
