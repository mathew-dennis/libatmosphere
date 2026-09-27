cmake_minimum_required(VERSION 3.16)
project(atmosphere VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Quick)
find_package(PkgConfig)
pkg_check_modules(CUTIESTORE cutiestore REQUIRED IMPORTED_TARGET)

qt_standard_project_setup()

set(lib_SOURCES
    src/atmosphere.cpp src/atmosphere.h
    src/paletteextractor.cpp src/paletteextractor.h)

qt6_add_library(atmosphere
    SHARED ${lib_SOURCES}
)

set_target_properties(atmosphere PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(atmosphere PROPERTIES SOVERSION 0)
set_target_properties(atmosphere PROPERTIES PUBLIC_HEADER src/atmosphere.h)

target_link_libraries(atmosphere PUBLIC
    Qt::Core
    Qt::Quick
    PkgConfig::CUTIESTORE
)

configure_file(src/atmosphere.pc.in atmosphere.pc @ONLY)

install(TARGETS atmosphere
    RUNTIME DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    BUNDLE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(FILES ${CMAKE_BINARY_DIR}/atmosphere.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
