########################################################################
# Project setup
########################################################################

cmake_minimum_required(VERSION 2.8)
project(etisnoop CXX)

# Select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Set the version information here
set(VERSION_INFO_MAJOR_VERSION 0)
set(VERSION_INFO_API_COMPAT    0)
set(VERSION_INFO_MINOR_VERSION 0)
set(VERSION_INFO_MAINT_VERSION git)


########################################################################
# Compiler specific setup
########################################################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -std=c++11")


########################################################################
# Find build dependencies
########################################################################

# libfaad2
find_package(Faad REQUIRED)
include_directories(${FAAD_INCLUDE_DIRS})

# libfec
find_package(Fec REQUIRED)
include_directories(${FEC_INCLUDE_DIRS})

########################################################################
# Setup apps
########################################################################

list(APPEND etisnoop_sources
    dabplussnoop.cpp
    etiinput.cpp
    etisnoop.cpp
    faad_decoder.cpp
    fig0_0.cpp
    fig0_1.cpp
    fig0_2.cpp
    fig0_3.cpp
    fig0_5.cpp
    fig0_6.cpp
    fig0_8.cpp
    fig0_9.cpp
    fig0_10.cpp
    fig0_11.cpp
    fig0_13.cpp
    fig0_14.cpp
    fig0_16.cpp
    fig0_17.cpp
    fig0_18.cpp
    fig0_19.cpp
    fig0_21.cpp
    fig0_22.cpp
    fig0_24.cpp
    fig0_25.cpp
    fig0_26.cpp
    fig0_27.cpp
    fig0_28.cpp
    fig0_31.cpp
    figs.cpp
    firecode.c
    lib_crc.c
    rsdecoder.cpp
    tables.cpp
    utils.cpp
    wavfile.c
)

set_source_files_properties(${etisnoop_sources} PROPERTIES LANGUAGE "CXX")
add_executable(etisnoop ${etisnoop_sources})
target_link_libraries(etisnoop ${FAAD_LIBRARIES} ${FEC_LIBRARIES})

install(TARGETS etisnoop DESTINATION bin)


########################################################################
# Create uninstall target
########################################################################

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

