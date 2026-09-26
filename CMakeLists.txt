cmake_minimum_required(VERSION 3.21)

project(NovaPad
    VERSION 1.0.0
    DESCRIPTION "A modern Notepad / Notepad++ style text editor built with C++ and Qt"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Keep executables out of the per-config subdirectory on multi-config generators
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(cfg DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${cfg} ${CMAKE_BINARY_DIR}/bin)
endforeach()

find_package(Qt6 6.4 COMPONENTS Widgets PrintSupport REQUIRED)

add_executable(novapad WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/editor.h
    src/editor.cpp
    src/findpanel.h
    src/findpanel.cpp
    src/highlighter.h
    src/highlighter.cpp
    src/languages.h
    src/languages.cpp
    src/themes.h
    src/themes.cpp
    src/icons.h
    src/icons.cpp
    src/search.h
    src/search.cpp
    src/fileio.h
    src/fileio.cpp
    resources/resources.qrc
)

if(WIN32)
    target_sources(novapad PRIVATE resources/novapad.rc)
endif()

target_link_libraries(novapad PRIVATE
    Qt6::Widgets
    Qt6::PrintSupport
)

target_compile_definitions(novapad PRIVATE
    NOVAPAD_VERSION="${PROJECT_VERSION}"
)

if(APPLE)
    set_target_properties(novapad PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "NovaPad"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.siktir.novapad"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
    )
endif()

install(TARGETS novapad
    BUNDLE DESTINATION .
    RUNTIME DESTINATION .
)
