# pico-enc28j60

[![builds.sr.ht status](https://builds.sr.ht/~krystianch/pico-enc28j60.svg)](https://builds.sr.ht/~krystianch/pico-enc28j60?)

Minimal enc28j60 library for use with the Pico SDK

## What it is

A low-level library that provides a configuration struct and functions for:

* soft resetting the IC,
* initializing the IC,
* ethernet packet reception,
* ethernet packet transmission.

## What it isn't

TCP/IP stack, socket, HTTP implementation.

BUT it includes example code of a [TCP echo server](tcp_echo.c) based on lwip.

## Documentation

See [include/pico/enc28j60.h](include/pico/enc28j60.h) for Doxygen style comments.

See [tcp_echo.c](tcp_echo.c) for example program.

## Building the TCP echo example

The following assumes that you have [pico-sdk](https://github.com/raspberrypi/pico-sdk) and [pico-extras](https://github.com/raspberrypi/pico-extras) cloned to `~/.local/src`.
Substitute your own paths below if your setup is different.

```bash
mkdir build
cd build
cmake -DPICO_SDK_PATH=~/.local/src/pico-sdk -DPICO_EXTRAS_PATH=~/.local/src/pico-extras -DPICO_ENC28J60_EXAMPLES_ENABLED=true ..
make  # creates tcp_echo.uf2
```

## Library usage

General library usage DOES NOT require [pico-extras](https://github.com/raspberrypi/pico-extras), just [pico-sdk](https://github.com/raspberrypi/pico-sdk).

You can add pico-enc28j60 to your project similarly to the SDK (copying [external/pico_enc28j60_import.cmake](external/pico_enc28j60_import.cmake) into your project)
having set the `PICO_ENC28J60_PATH` variable in your environment or via cmake variable.

```cmake
cmake_minimum_required(VERSION 3.12)

# Pull in PICO SDK (must be before project)
include(pico_sdk_import.cmake)

# We also need PICO ENC28J60
include(pico_enc28j60_import.cmake)

project(example_project)
set(CMAKE_C_STANDARD 11)
```
