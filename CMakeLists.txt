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

    set(LWIP_TEST_PATH "src/core/tcp.c")
    set(LWIP_PATH ${PICO_EXTRAS_PATH}/lib/lwip)

    set(PICO_ENC28J60_SRC src/enc28j60.c)
    set(PICO_ENC28J60_LIBS pico_stdlib hardware_spi)
    if (EXISTS ${LWIP_PATH}/${LWIP_TEST_PATH})
        message("lwIP available at ${LWIP_PATH}/${LWIP_TEST_PATH}; TCP/IP support is available.")
        set(PICO_ENC28J60_SRC ${PICO_ENC28J60_SRC} src/ethernetif.c)
        set(PICO_ENC28J60_LIBS ${PICO_ENC28J60_LIBS} lwip)
    endif()

    add_library(pico_enc28j60 INTERFACE)
    target_sources(pico_enc28j60 INTERFACE ${PICO_ENC28J60_SRC})
    target_link_libraries(pico_enc28j60 INTERFACE ${PICO_ENC28J60_LIBS})
    target_include_directories(pico_enc28j60 INTERFACE include)

    if (PICO_ENC28J60_EXAMPLES_ENABLED)
        add_executable(lwip_integration src/examples/lwip_integration.c ${LWIP_PATH}/contrib/apps/tcpecho_raw/tcpecho_raw.c)
        target_link_libraries(lwip_integration PRIVATE pico_enc28j60)
        target_include_directories(lwip_integration PRIVATE ${LWIP_PATH}/contrib/apps/tcpecho_raw include/pico/enc28j60/examples)
        message(${LWIP_PATH}/contrib/apps/tcpecho_raw)
        pico_add_extra_outputs(lwip_integration)
    endif ()

endif ()