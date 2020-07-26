CMAKE_MINIMUM_REQUIRED(VERSION 3.2 FATAL_ERROR)

PROJECT(gdbstub
    LANGUAGES C
    DESCRIPTION "GDB Remote Serial Protocol stub in C"
    VERSION 1.1.1)

###
### Install
###

ADD_LIBRARY(gdbstub INTERFACE)

TARGET_INCLUDE_DIRECTORIES(
    gdbstub INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>
)

INSTALL(
    TARGETS gdbstub
    EXPORT gdbstubTargets
    INCLUDES DESTINATION include
)

INCLUDE(CMakePackageConfigHelpers)

WRITE_BASIC_PACKAGE_VERSION_FILE(
    ${CMAKE_BINARY_DIR}/gdbstubConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

INSTALL(
    EXPORT gdbstubTargets
    NAMESPACE gdbstub::
    DESTINATION lib/cmake/gdbstub
)

CONFIGURE_PACKAGE_CONFIG_FILE(
    ${CMAKE_SOURCE_DIR}/cmake/gdbstubConfig.cmake.in
    ${CMAKE_BINARY_DIR}/gdbstubConfig.cmake
    INSTALL_DESTINATION lib/cmake/gdbstub
)

INSTALL(
    FILES 
        ${CMAKE_BINARY_DIR}/gdbstubConfigVersion.cmake
        ${CMAKE_BINARY_DIR}/gdbstubConfig.cmake
    DESTINATION lib/cmake/gdbstub
)

INCLUDE(FindPkgConfig)

IF(PKG_CONFIG_FOUND)
    CONFIGURE_FILE(
        ${CMAKE_SOURCE_DIR}/cmake/gdbstub.pc.in
        ${CMAKE_BINARY_DIR}/gdbstub.pc
        @ONLY
    )

    INSTALL(
        FILES ${CMAKE_BINARY_DIR}/gdbstub.pc
        DESTINATION share/pkgconfig
    )
ENDIF()

INSTALL(
    DIRECTORY ${CMAKE_SOURCE_DIR}/include/
    DESTINATION include
)
