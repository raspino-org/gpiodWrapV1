cmake_minimum_required(VERSION 3.16) # Raspbian 10 Buster / Bookworm kompatibel

project(gpiodWrapV1 VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wno-psabi -pthread")

find_library(GPIOD_LIBRARY gpiod)

if(NOT GPIOD_LIBRARY)
    message(WARNING "libgpiod not found. Skipping build.\n -> sudo apt install libgpiod-dev")
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GPIOD libgpiod>=1.6.0)

    if(NOT GPIOD_FOUND)
        message(FATAL_ERROR "libgpiod >= 1.6.0 required but not found!\n -> Installed version: ${GPIOD_VERSION}\n -> Please install: sudo apt install libgpiod-dev")
    else()

        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

        option(GPIODWRAPV1_DISABLE_SIGNALS "Disable built-in signal handling and watchdog thread" OFF)

        add_library(gpiodWrapV1 INTERFACE)

        target_link_libraries(gpiodWrapV1 INTERFACE ${GPIOD_LIBRARIES})
        target_include_directories(gpiodWrapV1 INTERFACE ${GPIOD_INCLUDE_DIRS})

        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.23")
            target_sources(gpiodWrapV1
                INTERFACE
                FILE_SET HEADERS
                TYPE HEADERS
                BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
                FILES gpiodWrapV1.hpp
            )
        else()
            target_include_directories(gpiodWrapV1 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
            target_sources(gpiodWrapV1 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/gpiodWrapV1.hpp)
        endif()

        if(GPIODWRAPV1_DISABLE_SIGNALS)
            message(STATUS "SIGNAL-HANDLING DISABLE")
            target_compile_definitions(gpiodWrapV1 INTERFACE GPIODWRAPV1_NO_SIGNALS)
        else()
            message(STATUS "SIGNAL-HANDLING ENABLE")
        endif()

        add_subdirectory(examples)
    endif()
endif()
