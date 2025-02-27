# Privact - Client

The client-side daemon for Privact.

It has two functionalities:

1. Continuously gathering personal data (at first only system health data).
2. Responding to federated surveys (polled from [../server](../server)).

## Status

Heavily work in progress, the current goal is to get it to a proof of concept
stage.

## Development

### Requirements

For the moment, you'll need to manually install a few dependencies:

1. A C++ toolchain (e.g. GCC, LLVM or MSVC) - note that at the time of writing,
   you will have to use Clang for the linting to work (see below).
2. [CMake](https://cmake.org/)
3. The [Qt framework](https://www.qt.io/product/qt6), version 6.2.4 (or later,
   presumably)
4. [GPGME](https://gnupg.org/software/gpgme/index.html)
5. [Qt Designer](https://doc.qt.io/qt-6/qtdesigner-manual.html)
6. [clang-format](https://clang.llvm.org/docs/ClangFormat.html) and
   [clang-tidy](https://clang.llvm.org/extra/clang-tidy/), version 14 (later
   versions format code differently, unfortunately)
7. [botan](https://botan.randombit.net/)

#### Debian/Ubuntu

    sudo apt install build-essential clang cmake qt6-base-dev clang-format clang-tidy designer-qt6
    echo 'export CXX=/usr/bin/clang++' >> ~/.profile
    . ~/.profile

### Building

    make

### Running the tests

    make && make test

### Linting

We use linting to improve maintainability and to reduce nit picking, please run
it before committing any changes:

    make lint

To comply with the formatting rules, run:

    make format

### Running the daemon

    make run-daemon

There are a few environment variables you can set to configure the daemon:

- `PRIVACT_CLIENT_SERVER_URL` - point to the base URL of the server, without
  trailing slash, e.g. `http://localhost:8000`.
- `PRIVACT_CLIENT_ENABLE_E2E` - set to `1` to enable experimental end to end
  encryption.

### Running the UI

    make run-ui

### Submitting data

The daemon receives data from suppliers via D-Bus. See
[example_supplier](example_supplier) for an example supplier you can use to
submit arbitrary data.
