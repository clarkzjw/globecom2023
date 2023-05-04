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
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_MODULE_PATH} ./cmake)

#add_compile_options(-Wall -Wno-unknown-pragmas -Wno-reorder -Wno-sign-compare -Wno-unused-variable)
add_compile_options(-Wpedantic -Werror)
add_compile_options(-fPIC)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

message(STATUS "CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}")
LIST(APPEND CMAKE_PREFIX_PATH "${CMAKE_INSTALL_PREFIX}/../Chrono/lib")

find_package(Picoquic REQUIRED)
message(STATUS "Picoquic_INCLUDE_DIRS: ${Picoquic_INCLUDE_DIRS}" )
message(STATUS "Picoquic_LIBRARIES: ${Picoquic_LIBRARIES}" )

find_package(PTLS REQUIRED)
message(STATUS "PTLS_INCLUDE_DIRS: ${PTLS_INCLUDE_DIRS}" )
message(STATUS "PTLS_LIBRARIES: ${PTLS_LIBRARIES}" )

find_package(OpenSSL REQUIRED)
message(STATUS "OPENSSL_VERSION: ${OPENSSL_VERSION}")
message(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")


file(GLOB_RECURSE tplayer_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}
        picoquic/client.h
        picoquic/client.c
        player.cpp
        tplayer.cpp)

file(GLOB_RECURSE server_SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}
        picoquic/server.c
        picoquic/*.h)

include_directories(
        ./picoquic
        ./config
        ${Picoquic_INCLUDE_DIRS}
        ${PTLS_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
)

add_library(player SHARED
        ${tplayer_SOURCE}
        )

target_link_libraries(player
        ${ZLIB_LIBRARIES}
        ${Picoquic_LIBRARIES}
        ${PTLS_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        -lpthread)

add_executable(tserver
        ${server_SOURCE})

target_link_libraries(tserver
        ${ZLIB_LIBRARIES}
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

add_custom_target(
        c
        COMMAND rm -rf qlog/*.qlog qlog/*.log tmp/*.svc)
