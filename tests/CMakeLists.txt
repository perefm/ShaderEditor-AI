add_library(Catch2WithMain STATIC support/catch_main.cpp)
target_include_directories(Catch2WithMain PUBLIC ${PROJECT_SOURCE_DIR}/third_party)

add_executable(shader_editor_tests
    unit/test_example_shaders.cpp
    unit/test_preview_camera.cpp
    unit/test_render_session.cpp
    unit/test_shader_file_service.cpp
    unit/test_uniform_introspection.cpp
    unit/test_workspace_models.cpp
)

target_link_libraries(shader_editor_tests PRIVATE shader_editor_core Catch2WithMain)
target_compile_definitions(shader_editor_tests PRIVATE SHADEREDITOR_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME shader_editor_tests COMMAND shader_editor_tests)
