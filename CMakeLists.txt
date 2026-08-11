# CMake build for libui.
#
# Adapted from petabyt/libui-dev's CMakeLists.txt. The meson build remains the
# reference build; this lets us build without being tied to what's available on
# meson's wrapdb.

cmake_minimum_required(VERSION 3.15)

project(libui LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    enable_language(OBJC)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
endif()

option(LIBUI_COMPILE_EXAMPLES "Compile example programs and tests" OFF)
option(LIBUI_INCLUDE_INSTALL "Include install() declaration" OFF)
option(LIBUI_COMPILE_GTK3 "Compile GTK3 on Linux" ON)
option(LIBUI_BUILD_SHARED "Build libui as a shared library" OFF)

if(LIBUI_BUILD_SHARED)
    set(LIBUI_LIB_TYPE SHARED)
else()
    set(LIBUI_LIB_TYPE STATIC)
endif()

file(GLOB LIBUI_COMMON CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/common/*.c)
list(FILTER LIBUI_COMMON EXCLUDE REGEX ".*OLD_table\\.c$")

if(MSVC)
    set(LIBUI_COMMON_FLAGS
        /W3
        /wd4100
        /bigobj
        /D_FILE_OFFSET_BITS=64
        /Dlibui_EXPORTS
    )
    if(NOT LIBUI_BUILD_SHARED)
        list(APPEND LIBUI_COMMON_FLAGS /D_UI_STATIC)
    endif()
else()
    set(LIBUI_COMMON_FLAGS
        -fvisibility=hidden
        -fdiagnostics-color=always
        -D_FILE_OFFSET_BITS=64
        -Wall
        -Winvalid-pch
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
        -Wno-switch
        -Dlibui_EXPORTS
    )
    if(NOT LIBUI_BUILD_SHARED)
        list(APPEND LIBUI_COMMON_FLAGS -D_UI_STATIC)
    endif()
endif()

if(WIN32)
    file(GLOB LIBUI_WIN CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/windows/*.cpp)

    add_library(ui_win32 ${LIBUI_LIB_TYPE} ${LIBUI_COMMON} ${LIBUI_WIN})
    target_include_directories(ui_win32
        PRIVATE ${PROJECT_SOURCE_DIR}/windows ${PROJECT_SOURCE_DIR}
        PUBLIC ${PROJECT_SOURCE_DIR})
    target_compile_options(ui_win32 PRIVATE ${LIBUI_COMMON_FLAGS})
    target_link_libraries(ui_win32 PUBLIC
        user32 kernel32 gdi32 comctl32 uxtheme msimg32 comdlg32 d2d1 dwrite
        ole32 oleaut32 oleacc uuid windowscodecs)
    if(MINGW)
        # don't require shipping the MinGW-w64 DLLs
        target_link_libraries(ui_win32 PUBLIC -static -static-libgcc -static-libstdc++)
    endif()
    set(LIBUI_LIB ui_win32)
elseif(APPLE)
    file(GLOB LIBUI_DARWIN CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/darwin/*.m)
    list(FILTER LIBUI_DARWIN EXCLUDE REGEX ".*OLD_table\\.m$")

    add_library(ui_darwin ${LIBUI_LIB_TYPE} ${LIBUI_COMMON} ${LIBUI_DARWIN})
    target_include_directories(ui_darwin
        PRIVATE ${PROJECT_SOURCE_DIR}/darwin ${PROJECT_SOURCE_DIR}
        PUBLIC ${PROJECT_SOURCE_DIR})
    target_compile_options(ui_darwin PRIVATE ${LIBUI_COMMON_FLAGS})
    target_link_libraries(ui_darwin PUBLIC
        "$<LINK_LIBRARY:FRAMEWORK,CoreFoundation,Foundation,AppKit,CoreText,CoreGraphics>")
    set(LIBUI_LIB ui_darwin)
elseif(UNIX)
    if(LIBUI_COMPILE_GTK3)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(GTK3 IMPORTED_TARGET GLOBAL gtk+-3.0)

        file(GLOB LIBUI_GTK3 CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/unix/*.c)
        list(FILTER LIBUI_GTK3 EXCLUDE REGEX ".*OLD_table\\.c$")

        add_library(ui_gtk3 ${LIBUI_LIB_TYPE} ${LIBUI_COMMON} ${LIBUI_GTK3})
        target_include_directories(ui_gtk3
            PRIVATE ${PROJECT_SOURCE_DIR}/unix ${PROJECT_SOURCE_DIR}
            PUBLIC ${PROJECT_SOURCE_DIR})
        target_compile_options(ui_gtk3 PRIVATE ${LIBUI_COMMON_FLAGS})
        target_link_libraries(ui_gtk3 PUBLIC PkgConfig::GTK3 m)
        set(LIBUI_LIB ui_gtk3)
    else()
        message(FATAL_ERROR "no Unix backend selected: enable LIBUI_COMPILE_GTK3")
    endif()
else()
    message(FATAL_ERROR "platform not supported")
endif()

if(WIN32)
    # The exes embed the app manifest via their resources.rc, so keep the
    # toolchain from embedding its own default one too:
    #
    #  - cl.exe/clang-cl (link.exe): disable manifest generation, otherwise the
    #    linker emits (or replaces ours) an extra RT_MANIFEST resource.
    #  - plain clang (MSVC ABI): CMake's Windows-Clang.cmake bakes a
    #    `-Xlinker /MANIFEST:EMBED` into the link rule, which makes lld-link
    #    embed a second manifest and fail the link (as seen in CI).
    if(MSVC)
        add_link_options(/MANIFEST:NO)
    endif()
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" AND CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "GNU")
        string(REPLACE " -Xlinker /MANIFEST:EMBED" "" CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINK_EXECUTABLE}")
        string(REPLACE " -Xlinker /MANIFEST:EMBED" "" CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINK_EXECUTABLE}")
        string(REPLACE " -Xlinker /MANIFEST:EMBED" "" CMAKE_C_LINK_SHARED_LIBRARY "${CMAKE_C_LINK_SHARED_LIBRARY}")
        string(REPLACE " -Xlinker /MANIFEST:EMBED" "" CMAKE_CXX_LINK_SHARED_LIBRARY "${CMAKE_CXX_LINK_SHARED_LIBRARY}")
    endif()
endif()

add_library(LibUI::LibUI ALIAS ${LIBUI_LIB})
set_target_properties(${LIBUI_LIB} PROPERTIES OUTPUT_NAME "ui")
set(LIBUI_LINK_NAME "ui")

if(LIBUI_INCLUDE_INSTALL)
    configure_file(libui.pc.in ui.pc @ONLY)
    include(GNUInstallDirs)
    install(TARGETS ${LIBUI_LIB})
    install(
        FILES
        ui.h
        ui_windows.h
        ui_darwin.h
        ui_unix.h
        DESTINATION include/ui
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/ui.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()

if(LIBUI_COMPILE_EXAMPLES)
    # The unit tests need cmocka. Prefer the copy that meson's wrap already
    # extracts into subprojects/ (see subprojects/cmocka.wrap); when it is
    # absent (e.g. a fresh CI checkout), download the same source the wrap
    # points at.
    set(CMOCKA_VERSION 2.0.2)
    set(CMOCKA_DIR subprojects/cmocka-${CMOCKA_VERSION})
    set(CMOCKA_SHA256 39f92f366bdf3f1a02af4da75b4a5c52df6c9f7e736c7d65de13283f9f0ef416)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "build shared libraries" FORCE)
    set(UNIT_TESTING OFF CACHE BOOL "build cmocka's own unit tests" FORCE)
    set(WITH_EXAMPLES OFF CACHE BOOL "build cmocka's example programs" FORCE)
    if(EXISTS ${PROJECT_SOURCE_DIR}/${CMOCKA_DIR}/CMakeLists.txt)
        add_subdirectory(${PROJECT_SOURCE_DIR}/${CMOCKA_DIR}
            ${CMAKE_CURRENT_BINARY_DIR}/cmocka)
    else()
        include(FetchContent)
        FetchContent_Declare(cmocka
            URL
                https://cmocka.org/files/2.0/cmocka-${CMOCKA_VERSION}.tar.xz
            URL_HASH SHA256=${CMOCKA_SHA256}
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        FetchContent_MakeAvailable(cmocka)
    endif()

    function(add_ui_exe name rc)
        set(_ex_sources ${ARGN})
        if(WIN32)
            list(APPEND _ex_sources ${rc})
        endif()
        add_executable(${name} ${_ex_sources})
        target_link_libraries(${name} PRIVATE LibUI::LibUI)
        if(WIN32)
            # Always embed the Common Controls 6 manifest (the *.static.manifest
            # selection) so uiInit() works. The manifest-less *.manifest files
            # are an upstream debug aid for exposing theming bugs and would make
            # InitCommonControlsEx() fail in shared builds.
            target_compile_definitions(${name} PRIVATE _UI_STATIC)
            # GUI subsystem so the app doesn't pop up a console window behind
            # itself. main() still works as the entry point: MSVC links with
            # /ENTRY:mainCRTStartup below, and MinGW's -mwindows keeps calling
            # main().
            set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE TRUE)
        endif()
        if(MSVC)
            target_link_options(${name} PRIVATE /ENTRY:mainCRTStartup)
        endif()
    endfunction()

    # interactive test runner
    add_ui_exe(tester test/resources.rc
        test/drawtests.c test/images.c test/main.c test/menus.c
        test/page1.c test/page2.c test/page3.c test/page4.c test/page5.c
        test/page6.c test/page7.c test/page7a.c test/page7b.c test/page7c.c
        test/page10.c test/page11.c test/page12.c test/page13.c test/page14.c
        test/page15.c test/page16.c test/page17.c test/spaced.c)
    target_include_directories(tester PRIVATE ${PROJECT_SOURCE_DIR}/test)

    # cmocka unit tests
    add_ui_exe(unit test/unit/resources.rc
        test/unit/main.c test/unit/init.c test/unit/slider.c test/unit/spinbox.c
        test/unit/label.c test/unit/button.c test/unit/combobox.c
        test/unit/editablecombobox.c
        test/unit/checkbox.c test/unit/radiobuttons.c test/unit/tab.c
        test/unit/scroll.c
        test/unit/entry.c test/unit/menu.c test/unit/progressbar.c
        test/unit/drawmatrix.c test/unit/attrstr.c test/unit/window.c
        test/unit/tooltip.c)
    target_include_directories(unit PRIVATE ${PROJECT_SOURCE_DIR}/test/unit)
    target_link_libraries(unit PRIVATE cmocka)
    # the unit runner reports results on stdout; keep it console-subsystem
    set_target_properties(unit PROPERTIES WIN32_EXECUTABLE FALSE)
    enable_testing()
    add_test(NAME unit COMMAND unit)

    # headless control smoke tests
    add_ui_exe(qa test/qa/resources.rc
        test/qa/qa.c test/qa/main.c test/qa/button.c test/qa/checkbox.c
        test/qa/entry.c test/qa/label.c test/qa/radiobuttons.c
        test/qa/separator.c test/qa/spinbox.c test/qa/window.c)
    target_include_directories(qa PRIVATE ${PROJECT_SOURCE_DIR}/test/qa)

    add_ui_exe(hello-world examples/resources.rc examples/hello-world.c)
    add_ui_exe(window examples/resources.rc examples/window.c)
    add_ui_exe(button examples/resources.rc examples/button.c)
    add_ui_exe(label examples/resources.rc examples/label.c)
    add_ui_exe(checkbox examples/resources.rc examples/checkbox.c)
    add_ui_exe(entry examples/resources.rc examples/entry.c)
    add_ui_exe(multiline-entry examples/resources.rc examples/multiline-entry.c)
    add_ui_exe(box-layout examples/resources.rc examples/box-layout.c)
    add_ui_exe(form-layout examples/resources.rc examples/form-layout.c)
    add_ui_exe(grid-layout examples/resources.rc examples/grid-layout.c)
    add_ui_exe(tabs examples/resources.rc examples/tabs.c)
    add_ui_exe(group examples/resources.rc examples/group.c)
    add_ui_exe(control-destroy examples/resources.rc examples/control-destroy.c)
    add_ui_exe(combobox examples/resources.rc examples/combobox.c)
    add_ui_exe(editable-combobox examples/resources.rc examples/editable-combobox.c)
    add_ui_exe(radio-buttons examples/resources.rc examples/radio-buttons.c)
    add_ui_exe(spinbox examples/resources.rc examples/spinbox.c)
    add_ui_exe(slider examples/resources.rc examples/slider.c)
    add_ui_exe(progressbar examples/resources.rc examples/progressbar.c)
    add_ui_exe(separator examples/resources.rc examples/separator.c)
    add_ui_exe(date-picker examples/resources.rc examples/date-picker.c)
    add_ui_exe(time-picker examples/resources.rc examples/time-picker.c)
    add_ui_exe(color-button examples/resources.rc examples/color-button.c)
    add_ui_exe(font-button examples/resources.rc examples/font-button.c)
    add_ui_exe(menu examples/resources.rc examples/menu.c)
    add_ui_exe(menu-checkbox examples/resources.rc examples/menu-checkbox.c)
    add_ui_exe(open-file examples/resources.rc examples/open-file.c)
    add_ui_exe(open-folder examples/resources.rc examples/open-folder.c)
    add_ui_exe(save-file examples/resources.rc examples/save-file.c)
    add_ui_exe(message-box examples/resources.rc examples/message-box.c)
    add_ui_exe(error-message-box examples/resources.rc examples/error-message-box.c)
    add_ui_exe(timer examples/resources.rc examples/timer.c)
    add_ui_exe(queue-main examples/resources.rc examples/queue-main.c)
    add_ui_exe(should-quit examples/resources.rc examples/should-quit.c)
    add_ui_exe(controlgallery examples/resources.rc examples/controlgallery/main.c)
    add_ui_exe(histogram examples/resources.rc examples/histogram/main.c)
    add_ui_exe(drawtext examples/resources.rc examples/drawtext/main.c)
    add_ui_exe(drawbitmap examples/resources.rc examples/drawbitmap.c)
    add_ui_exe(datetime examples/resources.rc examples/datetime/main.c)

    find_package(Threads REQUIRED)
    add_ui_exe(cpp-multithread examples/resources.rc examples/cpp-multithread/main.cpp)
    target_link_libraries(cpp-multithread PRIVATE Threads::Threads)
endif()
