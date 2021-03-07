# This is a copy of <PICO_ENC28J60_PATH>/external/pico_enc28j60_import.cmake

# This can be dropped into an external project to help locate pico-enc28j60
# It should be include()ed prior to project()

if (DEFINED ENV{PICO_ENC28J60_PATH} AND (NOT PICO_ENC28J60_PATH))
    set(PICO_ENC28J60_PATH $ENV{PICO_ENC28J60_PATH})
    message("Using PICO_ENC28J60_PATH from environment ('${PICO_ENC28J60_PATH}')")
endif ()

if (NOT PICO_ENC28J60_PATH)
    if (PICO_SDK_PATH AND EXISTS "${PICO_SDK_PATH}/../pico-enc28j60")
        set(PICO_ENC28J60_PATH ${PICO_SDK_PATH}/../pico-enc28j60)
        message("Defaulting PICO_ENC28J60_PATH as sibling of PICO_SDK_PATH: ${PICO_ENC28J60_PATH}")
    else()
        message(FATAL_ERROR
                "PICO ENC28J60 location was not specified. Please set PICO_ENC28J60_PATH or set PICO_ENC28J60_FETCH_FROM_GIT to on to fetch from git."
                )
    endif()
endif ()

set(PICO_ENC28J60_PATH "${PICO_ENC28J60_PATH}" CACHE PATH "Path to the PICO ENC28J60")

get_filename_component(PICO_ENC28J60_PATH "${PICO_ENC28J60_PATH}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
if (NOT EXISTS ${PICO_ENC28J60_PATH})
    message(FATAL_ERROR "Directory '${PICO_ENC28J60_PATH}' not found")
endif ()

set(PICO_ENC28J60_PATH ${PICO_ENC28J60_PATH} CACHE PATH "Path to the PICO ENC28J60" FORCE)

add_subdirectory(${PICO_ENC28J60_PATH} pico_enc28j60)