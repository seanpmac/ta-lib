cmake_minimum_required(VERSION 3.18)
project(ta_perf C)

set(CMAKE_C_STANDARD 11)

# Find TA-Lib (assumes it's installed or in parent build)
find_path(TALIB_INCLUDE_DIR ta_libc.h
    HINTS
        ${CMAKE_SOURCE_DIR}/../../../include
        /usr/local/include
        /usr/include
)

find_library(TALIB_LIBRARY
    NAMES ta-lib libta-lib ta-lib-static
    HINTS
        ${CMAKE_SOURCE_DIR}/../../../build
        /usr/local/lib
        /usr/lib
)

if(NOT TALIB_INCLUDE_DIR OR NOT TALIB_LIBRARY)
    message(FATAL_ERROR "TA-Lib not found. Build the main library first or install system-wide.")
endif()

include_directories(${TALIB_INCLUDE_DIR})

# Platform-specific threading and timing libraries
if(WIN32)
    set(PLATFORM_LIBS "")
elseif(APPLE)
    set(PLATFORM_LIBS pthread)
else()
    set(PLATFORM_LIBS pthread rt)
endif()

# Source files
set(SOURCES
    ta_perf.c
    csv_loader.c
    timer.c
    runner.c
    reporter.c
)

# Build executable
add_executable(ta_perf ${SOURCES})
target_link_libraries(ta_perf ${TALIB_LIBRARY} ${PLATFORM_LIBS} m)

# Optional: OpenMP for parallel mode
find_package(OpenMP)
if(OpenMP_C_FOUND)
    target_link_libraries(ta_perf OpenMP::OpenMP_C)
    target_compile_definitions(ta_perf PRIVATE HAVE_OPENMP)
endif()

# Install target
install(TARGETS ta_perf DESTINATION bin)
