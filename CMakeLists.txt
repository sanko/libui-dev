# cmake build script for libui-dev
#
# libui-dev will follow the semantic versioning scheme. See https://semver.org/
#
# The official semantic version is stored in 'ui_version.h' using standard C macros:
#      #define LIBUI_VERSION_MAJOR <num>
#      #define LIBUI_VERSION_MINOR <num>
#      #define LIBUI_VERSION_PATCH <num>
#
# cmake parses these macros at configure time to set `PROJECT_VERSION`. DO NOT
# hardcode version numbers in build scripts.
#
# Building from an untagged Git commit automatically appends the short commit hash
# to `LIBUI_VERSION_STRING` (e.g., "0.4.2-a1b2c3d").
#
# This is controlled by the Cmake option `LIBUI_INCLUDE_GIT_HASH`.
# Example: `cmake -B build -DLIBUI_INCLUDE_GIT_HASH=OFF`
#
#      - AUTO (default) : Appends the git commit hash ONLY if the build is NOT
#                         on an exact tagged commit. If on an exact tag, it verifies
#                         that the tag version matches ui_version.h and warns
#                         if there is a mismatch.
#      - ON             : Always appends the Git commit hash regardless of tags.
#      - OFF            : Never appends the Git commit hash (clean semver only).
#
# When active, cmake passes `-DLIBUI_GIT_COMMIT=<hash>` as a PUBLIC compile
# definition on the library target. This propagates to 'ui_version.h' for both
# libui and any downstream executables linking against `LibUI::LibUI`.

cmake_minimum_required(VERSION 3.15)

# Parse version macros directly from ui_version.h
file(STRINGS ui_version.h LIBUI_VERSION_MAJOR_STRING REGEX "^#define LIBUI_VERSION_MAJOR ([0-9]+)$")
string(REGEX MATCH "[0-9]+" LIBUI_VERSION_MAJOR "${LIBUI_VERSION_MAJOR_STRING}")
file(STRINGS ui_version.h LIBUI_VERSION_MINOR_STRING REGEX "^#define LIBUI_VERSION_MINOR ([0-9]+)$")
string(REGEX MATCH "[0-9]+" LIBUI_VERSION_MINOR "${LIBUI_VERSION_MINOR_STRING}")
file(STRINGS ui_version.h LIBUI_VERSION_PATCH_STRING REGEX "^#define LIBUI_VERSION_PATCH ([0-9]+)$")
string(REGEX MATCH "[0-9]+" LIBUI_VERSION_PATCH "${LIBUI_VERSION_PATCH_STRING}")

set(PROJECT_VERSION "${LIBUI_VERSION_MAJOR}.${LIBUI_VERSION_MINOR}.${LIBUI_VERSION_PATCH}")
set(PROJECT_VERSION_MAJOR "${LIBUI_VERSION_MAJOR}")
set(PROJECT_VERSION_MINOR "${LIBUI_VERSION_MINOR}")
set(PROJECT_VERSION_PATCH "${LIBUI_VERSION_PATCH}")

project(libui VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

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

# Build Options
option(LIBUI_COMPILE_EXAMPLES "Compile example programs and tests" OFF)
option(LIBUI_INCLUDE_INSTALL "Include install() declaration" OFF)
option(LIBUI_COMPILE_GTK3 "Compile GTK3 on Linux" ON)
option(LIBUI_BUILD_SHARED "Build libui as a shared library" OFF)

# Git Version String Option (AUTO, ON, or OFF)
set(LIBUI_INCLUDE_GIT_HASH "AUTO" CACHE STRING "Include Git commit hash in version string (AUTO, ON, or OFF)")
set_property(CACHE LIBUI_INCLUDE_GIT_HASH PROPERTY STRINGS "AUTO" "ON" "OFF")

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

# Platform-specific Backends
if(WIN32)
    file(GLOB LIBUI_WIN CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/windows/*.cpp)

    add_library(ui_win32 ${LIBUI_LIB_TYPE} ${LIBUI_COMMON} ${LIBUI_WIN})
    target_include_directories(ui_win32
        PRIVATE ${PROJECT_SOURCE_DIR}/windows ${PROJECT_SOURCE_DIR}
        PUBLIC ${PROJECT_SOURCE_DIR})
    target_compile_options(ui_win32 PRIVATE ${LIBUI_COMMON_FLAGS})
    target_link_libraries(ui_win32 PUBLIC
        user32 kernel32 gdi32 comctl32 uxtheme msimg32 comdlg32 d2d1 dwrite
        ole32 oleaut32 oleacc uuid windowscodecs shell32)
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
    # Prevent toolchains from embedding duplicate app manifests
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

# Target Properties & Aliases
add_library(LibUI::LibUI ALIAS ${LIBUI_LIB})
set_target_properties(${LIBUI_LIB} PROPERTIES OUTPUT_NAME "ui")
set_target_properties(${LIBUI_LIB} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
set(LIBUI_LINK_NAME "ui")

# git Versioning and Tag Validation
if(NOT LIBUI_INCLUDE_GIT_HASH STREQUAL "OFF")
    find_package(Git QUIET)
    if(Git_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
        set(SHOULD_APPEND_HASH FALSE)

        # Check if HEAD is on an exact Git tag
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --exact-match --tags
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            RESULT_VARIABLE GIT_TAG_RESULT
            OUTPUT_VARIABLE GIT_TAG_NAME
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )

        if(GIT_TAG_RESULT EQUAL 0)
            # We are on a tagged commit -> Validate Git tag against ui_version.h
            string(REGEX MATCH "[0-9]+\\.[0-9]+\\.[0-9]+" TAG_VERSION "${GIT_TAG_NAME}")
            if(TAG_VERSION AND NOT TAG_VERSION STREQUAL PROJECT_VERSION)
                message(WARNING "Git tag '${GIT_TAG_NAME}' (version ${TAG_VERSION}) does not match ui_version.h version (${PROJECT_VERSION})!")
            endif()
        else()
            # Not on a tagged commit -> Append commit hash if mode is AUTO
            if(LIBUI_INCLUDE_GIT_HASH STREQUAL "AUTO")
                set(SHOULD_APPEND_HASH TRUE)
            endif()
        endif()

        # Explicit manual overrides for LIBUI_INCLUDE_GIT_HASH
        if(LIBUI_INCLUDE_GIT_HASH STREQUAL "ON")
            set(SHOULD_APPEND_HASH TRUE)
        elseif(LIBUI_INCLUDE_GIT_HASH STREQUAL "OFF")
            set(SHOULD_APPEND_HASH FALSE)
        endif()

        # Fetch short hash and pass as a PUBLIC compile definition
        if(SHOULD_APPEND_HASH)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                OUTPUT_VARIABLE LIBUI_GIT_COMMIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
            )
            if(LIBUI_GIT_COMMIT_HASH)
                target_compile_definitions(${LIBUI_LIB} PUBLIC LIBUI_GIT_COMMIT=${LIBUI_GIT_COMMIT_HASH})
            endif()
        endif()
    endif()
endif()

# Installation Configuration
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
        ui_version.h
        DESTINATION include/ui
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/ui.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
    )
endif()

if(LIBUI_COMPILE_EXAMPLES)
    # The unit tests need cmocka. The source of truth is the pinned submodule
    # at 3rdparty/cmocka (see .gitmodules); it must be initialized before
    # configuring, so no network access is needed at configure time.
    if(NOT EXISTS ${PROJECT_SOURCE_DIR}/3rdparty/cmocka/CMakeLists.txt)
        message(FATAL_ERROR
            "The cmocka submodule is missing. Initialize it with:\n"
            "    git submodule update --init\n"
            "then configure again.")
    endif()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "build shared libraries" FORCE)
    set(UNIT_TESTING OFF CACHE BOOL "build cmocka's own unit tests" FORCE)
    set(WITH_EXAMPLES OFF CACHE BOOL "build cmocka's example programs" FORCE)
    add_subdirectory(${PROJECT_SOURCE_DIR}/3rdparty/cmocka
        ${CMAKE_CURRENT_BINARY_DIR}/cmocka)

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
            # itself. main() still works as the entry point: MSVC-style
            # toolchains (MSVC, clang-cl, and GNU-style clang targeting the
            # MSVC ABI) need the CRT entry forced to mainCRTStartup below;
            # MinGW's -mwindows CRT resolves main() on its own.
            set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE TRUE)
            if(NOT MINGW)
                if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
                    target_link_options(${name} PRIVATE /ENTRY:mainCRTStartup)
                else()
                    target_link_options(${name} PRIVATE "-Wl,/ENTRY:mainCRTStartup")
                endif()
            endif()
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
        test/unit/drawmatrix.c test/unit/imagebuffer.c test/unit/attrstr.c test/unit/window.c
        test/unit/tooltip.c test/unit/dragdestination.c)
    target_include_directories(unit PRIVATE ${PROJECT_SOURCE_DIR}/test/unit)
    target_link_libraries(unit PRIVATE cmocka)
    # the unit runner reports results on stdout; keep it console-subsystem
    set_target_properties(unit PROPERTIES WIN32_EXECUTABLE FALSE)
    enable_testing()
    # each component runs as its own process so ctest reports failures per
    # component instead of one blob for the whole suite

    add_test(NAME unit-attrstr COMMAND unit attrstr)
    add_test(NAME unit-button COMMAND unit button)
    add_test(NAME unit-checkbox COMMAND unit checkbox)
    add_test(NAME unit-combobox COMMAND unit combobox)
    add_test(NAME unit-drawmatrix COMMAND unit drawmatrix)
    add_test(NAME unit-imagebuffer COMMAND unit imagebuffer)
    add_test(NAME unit-editablecombobox COMMAND unit editablecombobox)
    add_test(NAME unit-entry COMMAND unit entry)
    add_test(NAME unit-init COMMAND unit init)
    add_test(NAME unit-label COMMAND unit label)
    add_test(NAME unit-menu COMMAND unit menu)
    add_test(NAME unit-progressbar COMMAND unit progressbar)
    add_test(NAME unit-radiobuttons COMMAND unit radiobuttons)
    add_test(NAME unit-scroll COMMAND unit scroll)
    add_test(NAME unit-slider COMMAND unit slider)
    add_test(NAME unit-spinbox COMMAND unit spinbox)
    add_test(NAME unit-tab COMMAND unit tab)
    add_test(NAME unit-tooltip COMMAND unit tooltip)
    add_test(NAME unit-window COMMAND unit window)
    add_test(NAME unit-dragdestination COMMAND unit dragdestination)

    # headless control smoke tests
    add_ui_exe(qa test/qa/resources.rc
        test/qa/qa.c test/qa/main.c test/qa/button.c test/qa/checkbox.c
        test/qa/entry.c test/qa/label.c test/qa/radiobuttons.c
        test/qa/separator.c test/qa/dragdestination.c test/qa/spinbox.c test/qa/window.c)
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
    add_ui_exe(imagebuffer examples/resources.rc examples/imagebuffer.c)
    add_ui_exe(datetime examples/resources.rc examples/datetime/main.c)
    add_ui_exe(drag-drop examples/resources.rc examples/drag-drop/main.c)

    find_package(Threads REQUIRED)
    add_ui_exe(cpp-multithread examples/resources.rc examples/cpp-multithread/main.cpp)
    target_link_libraries(cpp-multithread PRIVATE Threads::Threads)
endif()
