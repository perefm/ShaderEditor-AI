# Regenerates the build-timestamp header on every build (not just on CMake configure).
# Invoked as a pre-build COMMAND so `string(TIMESTAMP ...)` here reflects the actual time
# the executable is compiled, matching the window title's intent of showing a "fresh build"
# marker rather than a value frozen from the last `cmake configure`.
string(TIMESTAMP SHADEREDITOR_VERSION_TIMESTAMP "%Y.%m.%d-%H:%M")
set(SHADEREDITOR_VERSION_HEADER_CONTENT
"#pragma once\n#define SHADEREDITOR_VERSION_TIMESTAMP \"${SHADEREDITOR_VERSION_TIMESTAMP}\"\n")

if(EXISTS "${SHADEREDITOR_VERSION_HEADER_PATH}")
    file(READ "${SHADEREDITOR_VERSION_HEADER_PATH}" SHADEREDITOR_EXISTING_VERSION_HEADER_CONTENT)
else()
    set(SHADEREDITOR_EXISTING_VERSION_HEADER_CONTENT "")
endif()

# Avoid rewriting the header (and so avoid forcing a relink) unless the minute actually changed.
if(NOT SHADEREDITOR_EXISTING_VERSION_HEADER_CONTENT STREQUAL SHADEREDITOR_VERSION_HEADER_CONTENT)
    file(WRITE "${SHADEREDITOR_VERSION_HEADER_PATH}" "${SHADEREDITOR_VERSION_HEADER_CONTENT}")
endif()
