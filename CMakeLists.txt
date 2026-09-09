cmake_minimum_required(VERSION 3.27)

# Default to a static vcpkg triplet so the produced executable does not depend on vcpkg
# shared libraries (glfw3.dll, assimp-*.dll, etc.) at runtime. The OS and MSVC runtime
# remain dynamic with the static-md triplet.
# This must be set before project() so the vcpkg toolchain picks it up. Users can still
# override it with -DVCPKG_TARGET_TRIPLET=... if a shared build is explicitly required.
if(NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "Vcpkg target triplet")
endif()

project(ShaderEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build project libraries as static libraries")

# Match vcpkg's "static-md" triplet: statically link every dependency library, but keep using the
# dynamic (DLL) MSVC C++ runtime so the executable still only needs the standard VC++
# redistributable, not a fully static CRT.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

# Placeholder for the very first configure; the real value used by the build is (re)generated
# on every build via the shader_editor_version_header custom target below, since CMake's own
# string(TIMESTAMP ...) call here is only re-evaluated when the project is reconfigured, not on
# every `cmake --build`, and would otherwise freeze the window title at the last configure time.
string(TIMESTAMP SHADEREDITOR_VERSION_TIMESTAMP "%Y.%m.%d-%H:%M")

enable_testing()

set(SHADEREDITOR_VCPKG_ROOT "")
if(DEFINED ENV{VCPKG_ROOT})
    set(SHADEREDITOR_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
endif()

set(SHADEREDITOR_VCPKG_INSTALLED_DIR "")
if(SHADEREDITOR_VCPKG_ROOT)
    set(SHADEREDITOR_VCPKG_INSTALLED_DIR "${SHADEREDITOR_VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")
endif()

if(EXISTS "${SHADEREDITOR_VCPKG_INSTALLED_DIR}")
    list(APPEND CMAKE_PREFIX_PATH
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/glfw3"
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/imgui"
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/glad"
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/assimp"
    )
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_path(STB_IMAGE_INCLUDE_DIR stb_image.h REQUIRED)

# Regenerates a small header with the current timestamp on every build (see
# cmake/GenerateVersionHeader.cmake for why this must run at build time rather than via
# string(TIMESTAMP ...) directly in this file, which is only re-evaluated on reconfigure).
set(SHADEREDITOR_VERSION_HEADER_DIR "${CMAKE_BINARY_DIR}/generated")
set(SHADEREDITOR_VERSION_HEADER_PATH "${SHADEREDITOR_VERSION_HEADER_DIR}/shadereditor_version.h")
file(MAKE_DIRECTORY "${SHADEREDITOR_VERSION_HEADER_DIR}")
add_custom_target(shader_editor_version_header ALL
    COMMAND ${CMAKE_COMMAND}
        "-DSHADEREDITOR_VERSION_HEADER_PATH=${SHADEREDITOR_VERSION_HEADER_PATH}"
        -P "${PROJECT_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake"
    BYPRODUCTS "${SHADEREDITOR_VERSION_HEADER_PATH}"
    COMMENT "Generating shadereditor_version.h with the current build timestamp"
)

add_library(shader_editor_core STATIC
    third_party/imgui_color_text_edit/TextEditor.cpp
    src/app/application/Application.cpp
    src/app/platform/WindowContext.cpp
    src/app/workspace/DiagnosticsState.cpp
    src/app/workspace/UniformState.cpp
    src/app/workspace/PreviewInteractionState.cpp
    src/app/workspace/WorkspaceController.cpp
    src/editor/ShaderEditorState.cpp
    src/rendering/geometry/BuiltInPrimitives.cpp
    src/rendering/geometry/PrimitiveLibrary.cpp
    src/rendering/models/AssimpModelLoader.cpp
    src/rendering/models/SkeletalAnimator.cpp
    src/rendering/models/ModelCameraResolver.cpp
    src/rendering/models/ModelInfoSummary.cpp
    src/rendering/opengl/PreviewRenderer.cpp
    src/rendering/opengl/PreviewCamera.cpp
    src/rendering/shaders/ShaderProgramService.cpp
    src/rendering/shaders/UniformIntrospectionService.cpp
    src/services/files/ShaderFileService.cpp
    src/services/shaders/PhoenixShaderParser.cpp
    src/ui/dockspace/DockspaceHost.cpp
    src/ui/panels/DiagnosticsPanel.cpp
    src/ui/panels/RenderViewPanel.cpp
    src/ui/panels/ModelInfoPanel.cpp
    src/ui/panels/ShaderEditorPanel.cpp
    src/ui/panels/ShaderErrorsPanel.cpp
    src/ui/panels/UniformsPanel.cpp
    src/ui/widgets/DocumentDialogs.cpp
)
add_dependencies(shader_editor_core shader_editor_version_header)

target_include_directories(shader_editor_core PUBLIC src third_party ${STB_IMAGE_INCLUDE_DIR} "${SHADEREDITOR_VERSION_HEADER_DIR}")
target_link_libraries(shader_editor_core PUBLIC glfw imgui::imgui glad::glad glm::glm opengl32 assimp::assimp)
if(WIN32)
    target_link_libraries(shader_editor_core PUBLIC comdlg32)
endif()

if(WIN32)
    add_executable(shader_editor WIN32
        src/app/application/main.cpp
        resources/shader_editor.rc
        resources/shader_editor.ico
    )
else()
    add_executable(shader_editor src/app/application/main.cpp)
endif()
target_link_libraries(shader_editor PRIVATE shader_editor_core)

# Ship the runtime assets beside the executable so example shaders are resolved from
# "<exe>/assets". This is a standalone ALL target (not a shader_editor POST_BUILD step) because
# MSBuild can skip POST_BUILD commands entirely when it decides shader_editor itself has nothing
# to relink (e.g. after only editing a .glsl file, with no C++ changes) - that meant shader edits
# were silently NOT being copied to build-vcpkg/<config>/assets, so the running exe kept loading
# the previous version. A custom target with no tracked OUTPUT always reruns on every build.
add_custom_target(shader_editor_copy_assets ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:shader_editor>/assets"
    COMMENT "Copying assets/ next to the shader_editor executable"
)
add_dependencies(shader_editor_copy_assets shader_editor)


add_subdirectory(tests)
