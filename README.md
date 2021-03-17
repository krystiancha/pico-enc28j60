# pico-enc28j60

[![builds.sr.ht status](https://builds.sr.ht/~krystianch/pico-enc28j60.svg)](https://builds.sr.ht/~krystianch/pico-enc28j60?)

enc28j60 driver and [lwIP](https://www.nongnu.org/lwip/) integration for the [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (RP2040 microcontroller).

## Requirements

* [pico-sdk (Raspberry Pi Pico SDK)](https://github.com/raspberrypi/pico-sdk)
* [pico-extras with lwIP submodule](https://github.com/raspberrypi/pico-sdk)

Remember to clone with submodules:

```bash
git clone --recurse-submodules https://github.com/raspberrypi/pico-extras.git
```

Or initialize submodules after a regular clone:

```bash
git clone https://github.com/raspberrypi/pico-extras.git
cd pico-extras
git submodule update --init --recursive
```

## Usage

You can follow these instructions to build a lwIP app.
The aim of this short tutorial is to successfully ping the device.

Clone this repository and the repositories mentioned in the [Requirements](#Requirements) section and note the clone paths.

Create a new empty directory that your project will reside in and `cd` to it.

```bash
mkdir picoping
cd picoping
```

Copy [src/examples/lwip_integration.c](src/examples/lwip_integration.c) and [include/pico/enc28j60/examples/lwipopts.h](include/pico/enc28j60/examples/lwipopts.h) and save it as `main.c` and `lwipopts.h` respectively in the new project root.

Remove the `tcpecho_raw_init();` line before the main loop as well as `#include "tcpecho_raw.h"`, because we only want a minimal example running.
Edit the configuration section in the same file to reflect your setup, paying extra attention to the static IP address, netmask and gateway address as this example app won't include support for DHCP (pico-extras do not support it yet).

Copy [external/pico_enc28j60_import.cmake](external/pico_enc28j60_import.cmake), [pico_extras_import.cmake](pico_extras_import.cmake) and [pico_sdk_import.cmake](pico_sdk_import.cmake) to your project's root and create a `CMakeLists.txt` with the following contents:

```cmake
cmake_minimum_required(VERSION 3.19)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
include(pico_enc28j60_import.cmake)

project(picoping)

set(CMAKE_C_STANDARD 11)

pico_sdk_init()

include_directories(.)

add_executable(picoping main.c)
target_link_libraries(picoping pico_stdlib lwip pico_enc28j60)
pico_add_extra_outputs(picoping)
```

Now let's build it.

Create a build directory and configure the project passing paths to required components to CMake.

```bash
mkdir build

cd build

cmake \
    -DPICO_SDK_PATH=/path/to/pico-sdk/ \
    -DPICO_EXTRAS_PATH=/path/to/pico-extras/ \
    -DPICO_ENC28J60_PATH=/path/to/pico-enc28j60/ \
    ..

make
```

The compilation process should succeed and you should have a `picoping.uf2` ready to load to your board.

After loading and rebooting you should be able to ping the IP address that you specified in the static configuration and get a response.
If this is not the case, check the configuration and UART as a lot of debug information is printed there.

You can treat this app as a base for developing your own lwIP app.
To make it easier, check out lwip-contrib as it contains examples such as tcp_echo_raw that are easy to integrate.
