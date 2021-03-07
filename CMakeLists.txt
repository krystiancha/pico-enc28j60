cmake_minimum_required(VERSION 3.13)

if (NOT TARGET _pico_enc28j60_inclusion_marker)
    add_library(_pico_enc28j60_inclusion_marker INTERFACE)

    include(pico_sdk_import.cmake)
    if (PICO_ENC28J60_EXAMPLES_ENABLED)
        include(pico_extras_import.cmake)
    endif ()

    project(pico_enc28j60)

    set(CMAKE_C_STANDARD 11)

    pico_sdk_init()

    pico_is_top_level_project(PICO_ENC28J60_TOP_LEVEL_PROJECT)

    add_library(pico_enc28j60_included INTERFACE)
    target_compile_definitions(pico_enc28j60_included INTERFACE
            -DPICO_ENC28J60=1
            )

    pico_add_platform_library(pico_enc28j60_included)

    if (NOT PICO_ENC28J60_PATH)
        set(PICO_ENC28J60_PATH ${CMAKE_CURRENT_LIST_DIR})
    endif ()
    set(PICO_ENC28J60_PATH "${PICO_ENC28J60_PATH}" CACHE PATH "Path to pico-enc28j60")

    add_library(pico_enc28j60 INTERFACE)
    target_sources(pico_enc28j60 INTERFACE enc28j60.c)
    target_link_libraries(pico_enc28j60 INTERFACE pico_stdlib hardware_spi)
    target_include_directories(pico_enc28j60 INTERFACE include)

    if (PICO_ENC28J60_EXAMPLES_ENABLED)
        add_executable(tcp_echo tcp_echo.c)
        target_link_libraries(tcp_echo PRIVATE pico_stdlib lwip pico_enc28j60)
        pico_add_extra_outputs(tcp_echo)
    endif ()

endif ()