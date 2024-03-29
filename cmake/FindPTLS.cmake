# - Try to find Picotls

find_path(PTLS_INCLUDE_DIR
        NAMES picotls/openssl.h
        HINTS ${PTLS_PREFIX}/include/picotls
        ${CMAKE_SOURCE_DIR}/../picotls/include
        ${CMAKE_BINARY_DIR}/../picotls/include
        ../../../picotls/include
        ../picotls/include/ )

set(PTLS_HINTS
        ${PTLS_PREFIX}/lib
        ${CMAKE_BINARY_DIR}/../picotls
        ../../../picotls/
        ../picotls)

find_library(PTLS_CORE_LIBRARY picotls-core HINTS ${PTLS_HINTS})
find_library(PTLS_OPENSSL_LIBRARY picotls-openssl HINTS ${PTLS_HINTS})
find_library(PTLS_FUSION_LIBRARY picotls-fusion HINTS ${PTLS_HINTS})
if(NOT PTLS_FUSION_LIBRARY)
    include(FindPackageHandleStandardArgs)
    # handle the QUIETLY and REQUIRED arguments and set PTLS_FOUND to TRUE
    # if all listed variables are TRUE
    find_package_handle_standard_args(PTLS REQUIRED_VARS
            PTLS_CORE_LIBRARY
            PTLS_OPENSSL_LIBRARY
            PTLS_INCLUDE_DIR)
    if(PTLS_FOUND)
        set(PTLS_LIBRARIES ${PTLS_CORE_LIBRARY} ${PTLS_OPENSSL_LIBRARY} )
        set(PTLS_INCLUDE_DIRS ${PTLS_INCLUDE_DIR})
        set(PTLS_WITH_FUSION_DEFAULT OFF)
    endif()
else()
    include(FindPackageHandleStandardArgs)
    # handle the QUIETLY and REQUIRED arguments and set PTLS_FOUND to TRUE
    # if all listed variables are TRUE
    find_package_handle_standard_args(PTLS REQUIRED_VARS
            PTLS_CORE_LIBRARY
            PTLS_OPENSSL_LIBRARY
            PTLS_FUSION_LIBRARY
            PTLS_INCLUDE_DIR)

    if(PTLS_FOUND)
        set(PTLS_LIBRARIES ${PTLS_CORE_LIBRARY} ${PTLS_OPENSSL_LIBRARY} ${PTLS_FUSION_LIBRARY})
        set(PTLS_INCLUDE_DIRS ${PTLS_INCLUDE_DIR})
        set(PTLS_WITH_FUSION_DEFAULT ON)
    endif()
endif()

mark_as_advanced(PTLS_LIBRARIES PTLS_INCLUDE_DIRS)
