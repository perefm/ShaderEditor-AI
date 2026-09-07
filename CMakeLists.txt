cmake_minimum_required(VERSION 3.27)

project(ShaderEditor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

string(TIMESTAMP SHADEREDITOR_VERSION_TIMESTAMP "%Y.%m.%d-%H:%M")

enable_testing()

set(SHADEREDITOR_VCPKG_ROOT "")
if(DEFINED ENV{VCPKG_ROOT})
    set(SHADEREDITOR_VCPKG_ROOT "$ENV{VCPKG_ROOT}")
endif()

set(SHADEREDITOR_VCPKG_INSTALLED_DIR "")
if(SHADEREDITOR_VCPKG_ROOT)
    set(SHADEREDITOR_VCPKG_INSTALLED_DIR "${SHADEREDITOR_VCPKG_ROOT}/installed/x64-windows")
endif()

if(EXISTS "${SHADEREDITOR_VCPKG_INSTALLED_DIR}")
    list(APPEND CMAKE_PREFIX_PATH
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/glfw3"
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/imgui"
        "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/share/glad"
    )
endif()

find_package(glfw3 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_path(STB_IMAGE_INCLUDE_DIR stb_image.h REQUIRED)

add_library(shader_editor_core
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
    src/rendering/opengl/PreviewRenderer.cpp
    src/rendering/opengl/PreviewCamera.cpp
    src/rendering/shaders/ShaderProgramService.cpp
    src/rendering/shaders/UniformIntrospectionService.cpp
    src/services/files/ShaderFileService.cpp
    src/services/shaders/PhoenixShaderParser.cpp
    src/ui/dockspace/DockspaceHost.cpp
    src/ui/panels/DiagnosticsPanel.cpp
    src/ui/panels/RenderViewPanel.cpp
    src/ui/panels/ShaderEditorPanel.cpp
    src/ui/panels/ShaderErrorsPanel.cpp
    src/ui/panels/UniformsPanel.cpp
    src/ui/widgets/DocumentDialogs.cpp
)

target_include_directories(shader_editor_core PUBLIC src third_party ${STB_IMAGE_INCLUDE_DIR})
target_link_libraries(shader_editor_core PUBLIC glfw imgui::imgui glad::glad glm::glm opengl32)
target_compile_definitions(shader_editor_core PRIVATE
    SHADEREDITOR_VERSION_TIMESTAMP="${SHADEREDITOR_VERSION_TIMESTAMP}"
)
if(WIN32)
    target_link_libraries(shader_editor_core PUBLIC comdlg32)
endif()

add_executable(shader_editor src/app/application/main.cpp)
target_link_libraries(shader_editor PRIVATE shader_editor_core)

if(EXISTS "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/debug/bin/glfw3.dll")
    add_custom_command(TARGET shader_editor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${SHADEREDITOR_VCPKG_INSTALLED_DIR}/debug/bin/glfw3.dll"
            "$<TARGET_FILE_DIR:shader_editor>/glfw3.dll"
    )
endif()

# Ship the runtime assets beside the executable so example shaders are resolved from "<exe>/assets".
add_custom_command(TARGET shader_editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:shader_editor>/assets"
)

add_subdirectory(tests)
