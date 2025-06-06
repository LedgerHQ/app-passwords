# app-passwords

## Quick summary

The Passwords application for Ledger Nano S and Nano X is available for download on the [Ledger Live](https://www.ledger.com/ledger-live/download).

This application demonstrates a Password Manager implemented with no support from the host - the passwords are typed from the Nano S interacting as a keyboard to the connected computer / phone.

## Usage

To create a password:

- Choose which kind of characters you want in this password (lowercase, uppercase, numbers, dashes, extra symbols)
- Enter a nickname for the new entry (for instance, "wikipedia.com").
  The device then derives a deterministic password from the device's seed and this nickname.

To type a password, just select it in your list of password.

If you want to add a lot of passwords, this process can be pretty painful. Instead of doing it manually, you can use the [backup tool](https://blog.ledger.com/passwords-backup/) to load a custom list of password nicknames.

### Application settings

In the application settings, the user can configure

- Which keyboard the device should emulate when typing a password (Qwerty, International Qwerty or Azerty).

- If the `Enter` key should be pressed automatically after typing a password (this is convenient for typing start-up password at encrypted servers without attaching display and keyboard, for instance).

## Backup

As passwords are deterministically derived, it's not a problem if you loose your device, as long as you remember the password nicknames and you still have you device recovery phrase to set up again the Passwords app on a new device.

Same applies when updating the device firmware or the application itself, the list of password nicknames won't be restored automatically, so make sure to save a backup using [this tool](https://blog.ledger.com/passwords-backup/).

These nicknames are not confidential (meaning, someone who finds them will not be able to retrieve your passwords without your [24-words recovery phrase](https://www.ledger.com/academy/crypto/what-is-a-recovery-phrase)), so you don't have to hide your backup like you did with your recovery phrase. Sending it to yourself by e-mail is fine.

## Password generation mechanism

- Metadatas are SHA-256 hashed

- The SHA-256 components are turned into 8 big endian uint32 | 0x80000000

- A private key and chain code are derived for secp256k1 over 0x80505744 / the path computed before

- The private key and chain code are SHA-256 hashed, the result is used as the entropy to seed an AES DRBG

- A password is generated by randomly choosing from a set of characters using the previously seeded DRBG

## Troobleshooting

- If you configured a password with some charset only, and you get unwanted characters when typing it, check that you have configured the application with the right keyboard. It must be configured like the keyboard settings of your operating systems to type correctly.

- If the keyboard is not recognized by your computer, have a look [here](https://support.ledger.com/hc/en-us/articles/115005165269-Fix-connection-issues)

## Tests

### Unit

#### Prerequisite

Be sure to have installed:

- CMake >= 3.10
- CMocka >= 1.1.5

and for code coverage generation:

- lcov >= 1.14

#### Overview

Unit tests are in `C` and uses `cmake` to build and `cmocka` as a library.
You will then need to compile the tests:

```bash
(cd tests/unit/ && \
 rm -rf build/ && \
 cmake -B build -H. && \
 make -C build)
```

You can then run the tests:

```bash
(cd tests/unit/ && \
 CTEST_OUTPUT_ON_FAILURE=1 make -C build test)
```

### Functional

Functional tests are written with Pytest. Before running them, you first need to compile the application with env variables `TESTING=1` and `POPULATE=1`:

```bash
make all TESTING=1 POPULATE=1
```

Then you can execute tests on speculos with:

```bash
pytest tests/functional
```

To run tests on a real device, load the app on it:

```bash
make load TESTING=1 POPULATE=1
```

Then open the app on your device and run:

```bash
pytest --hid
```

## Future work

This release is an early alpha - among the missing parts :

- Support of different password policies mechanisms

## Credits

This application uses

- MBED TLS AES DRBG implementation (https://tls.mbed.org/ctr-drbg-source-code)
