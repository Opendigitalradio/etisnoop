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
    lib_crc.c
    firecode.c
    faad_decoder.cpp
    wavfile.c
    etiinput.cpp
    etisnoop.cpp
    rsdecoder.cpp
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

