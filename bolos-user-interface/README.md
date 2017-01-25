# BOLOS User Interface

This repository contains a user interface library for
[BOLOS](http://ledger.readthedocs.io/en/latest/bolos/index.html) applications.
This library is currently targeted at only the [Ledger Nano
S](https://github.com/LedgerHQ/ledger-nano-s), though it is possible it may be
expanded to support other devices in the future.

This library shifts the GUI rendering which is usually done on the STM32F042
microcontroller unit in the device which manages peripherals over to the secure
ST31H320 chip where applications are loaded. This gives applications much
greater control over the rendering process.

In order to do this, the meat of the library is a
[blitting](https://en.wikipedia.org/wiki/Bit_blit) API which allows the
application to be rendered onto an internal display buffer (the "bottom" buffer)
before being sent to the MCU to be displayed. When the application is ready to
display the frame, `bui_flush()` may be called which transfers the bottom
display buffer onto the "top" display buffer, where it will wait to be sent to
the MCU. This allows the application to begin rendering the next frame while the
previous one is still being displayed. This technique is called
double-buffering.

The library's API is thoroughly documented in its header files. In order to
include this library in your project, simply include the files under the
`include/` directory and link to the source files in `src/`.

## Development Cycle

This repository will follow a Git branching model similar to that described in
[Vincent Driessen's *A successful Git branching
model*](http://nvie.com/posts/a-successful-git-branching-model/) and a
versioning scheme similar to that defined by [Semantic Versioning
2.0.0](http://semver.org/).

## License

This library is distributed under the terms of the very permissive [Zlib
License](https://opensource.org/licenses/Zlib). The exact text of this license
is reproduced in the `LICENSE.txt` file as well as at the top of every source
file in this repository.
