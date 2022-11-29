#########################################
#
# Author: 
#   Jinwei Zhao     <clarkzjw@uvic.ca>
#
#########################################

cmake_minimum_required(VERSION 3.2.0)
cmake_policy(SET CMP0003 NEW)
cmake_policy(SET CMP0071 NEW)
project(player C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH} ./cmake)
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS")  # Second macro definition is required to fix the error: ‘UINT32_MAX’ was not declared in this scope

# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Picoquic REQUIRED)
message(STATUS "Picoquic/include: ${Picoquic_INCLUDE_DIRS}" )
message(STATUS "Picoquic library: ${Picoquic_LIBRARIES}" )

find_package(PTLS REQUIRED)
message(STATUS "picotls/include: ${PTLS_INCLUDE_DIRS}" )
message(STATUS "picotls libraries: ${PTLS_LIBRARIES}" )

find_package(OpenSSL REQUIRED)
message(STATUS "root: ${OPENSSL_ROOT_DIR}")
message(STATUS "OpenSSL_VERSION: ${OPENSSL_VERSION}")
message(STATUS "OpenSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")

find_package(LibXml2 REQUIRED)
message(STATUS "LibXml2_INCLUDE_DIR: ${LIBXML2_INCLUDE_DIR}")

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(Eigen3 REQUIRED)
message(STATUS "Eigen3 INCLUDE_DIR: ${EIGEN3_INCLUDE_DIR}")

message(STATUS "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/../Chrono/lib")
find_package(Chrono
        COMPONENTS Postprocess
        CONFIG)

file(GLOB_RECURSE tplayer_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}
        libdash/*.cpp
        libdash/*.c
        picoquic/client.h
        picoquic/client.c
        mab.cpp
        mab/arm/*.hpp
        mab/bandit/*.hpp
        mab/bandit/*.cpp
        mab/policy/*.hpp
        minRTT.cpp
        rr.cpp
        player.cpp
        tplayer.cpp
        util/mpd.cpp
        util/exec.cpp)

file(GLOB_RECURSE server_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}
        picoquic/server.c
        picoquic/*.h)

include_directories(
        ./libdash/include
        ./picoquic
        ./util
        ./config
        ./mab
        ${CHRONO_INCLUDE_DIRS}
        ${EIGEN3_INCLUDE_DIR}
        ${LIBXML2_INCLUDE_DIR}
        ${Picoquic_INCLUDE_DIRS}
        ${PTLS_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
)

# Tell CMake to create the executable
add_executable(tplayer
        ${tplayer_SOURCE}
        ${libcurl_SOURCE})

add_executable(tserver
        ${server_SOURCE})

target_compile_options(tplayer PUBLIC ${CHRONO_CXX_FLAGS})
target_link_options(tplayer PUBLIC ${CHRONO_LINKER_FLAGS})

# For the linker
target_link_libraries(tplayer
        ${CHRONO_LIBRARIES}
        ${CURL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${Picoquic_LIBRARIES}
        ${PTLS_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        -lpthread)

target_link_libraries(tserver
        ${CHRONO_LIBRARIES}
        ${CURL_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${LIBXML2_LIBRARIES}
        ${Picoquic_LIBRARIES}
        ${PTLS_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        -lpthread)

# get all project files for formatting
file(GLOB_RECURSE CLANG_FORMAT_SOURCE_FILES *.c *.h *.cpp)

# Adds clangformat as target that formats all source files
add_custom_target(
        clangformat
        COMMAND clang-format
        -style=Webkit
        -i
        ${CLANG_FORMAT_SOURCE_FILES}
)