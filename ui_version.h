// 9 june 2026
//
// libui-dev follows the semantic versioning scheme. See https://semver.org/
// Our CMakeLists.txt can build the library with these values and the current
// git commit if building against a non-tagged commit.
#ifndef LIBUI_UI_VERSION_H
#define LIBUI_UI_VERSION_H

// Given a version number MAJOR.MINOR.PATCH, increment the:
//
// - MAJOR version when you make incompatible API changes
// - MINOR version when you add functionality in a backward compatible manner
// - PATCH version when you make backward compatible bug fixes
//
// Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format.

#define LIBUI_VERSION_MAJOR 0
#define LIBUI_VERSION_MINOR 4
#define LIBUI_VERSION_PATCH 2

// A single integer usable for comparisons, in the form MAJOR*10000+MINOR*100+PATCH
// (so 0.4.2 is 402). Computed here so it can never drift from the three macros.
#define LIBUI_VERSION_INT_HELPER(major, minor, patch) ((major) * 10000 + (minor) * 100 + (patch))

#define LIBUI_VERSION_INT LIBUI_VERSION_INT_HELPER(LIBUI_VERSION_MAJOR, LIBUI_VERSION_MINOR, LIBUI_VERSION_PATCH)

// C preprocessor stringification macros
#define LIBUI_STR_HELPER(x) #x
#define LIBUI_STR_EXPAND(x) LIBUI_STR_HELPER(x)

// Base version string (e.g., "0.4.2")
#define LIBUI_VERSION_STRING_BASE \
    LIBUI_STR_EXPAND(LIBUI_VERSION_MAJOR) "." \
    LIBUI_STR_EXPAND(LIBUI_VERSION_MINOR) "." \
    LIBUI_STR_EXPAND(LIBUI_VERSION_PATCH)

// If CMake passes in a Git commit, append it to the string for unstable builds
#ifdef LIBUI_GIT_COMMIT
    #define LIBUI_VERSION_STRING LIBUI_VERSION_STRING_BASE "-" LIBUI_STR_EXPAND(LIBUI_GIT_COMMIT)
#else
    #define LIBUI_VERSION_STRING LIBUI_VERSION_STRING_BASE
#endif

#define LIBUI_BUILD LIBUI_VERSION_INT

#endif
