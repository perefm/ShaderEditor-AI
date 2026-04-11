# Quickstart: Multiplatform Shader Editor

## Prerequisites

- CMake 3.27 or newer
- A C++20-capable compiler
- Python or package manager support only if used by the chosen dependency setup
- An OpenGL-capable desktop environment
- Visual Studio Code with CMake support for the preferred local workflow

## Build From VS Code

1. Open the repository root in Visual Studio Code.
2. Configure the CMake project using the default desktop preset.
3. Run the `cmake-build` task to build the `Debug` configuration.
4. Run the `ctest` task to execute the automated unit suite.
5. Launch the shader editor from the configured `Run Shader Editor` debug target.

## Build From CLI

1. Configure with `cmake --preset default`.
2. Build with `cmake --build --preset default`.
3. Test with `ctest --preset default`.
4. Launch `build-vcpkg/Debug/shader_editor.exe`.

## Cross-Platform Notes

- On Windows, the provided VS Code task and launch files target the `Debug`
  output produced by the Visual Studio generator.
- On Linux or macOS, regenerate the CMake build tree with the local compiler
  and keep the same source layout and task flow.
- The current source tree avoids hard-coded platform file separators and keeps
  workspace state in the local `build/` directory.

## Manual Smoke Validation

1. Launch the application.
2. Open a vertex shader and fragment shader pair.
3. Edit both files and save them.
4. Confirm the render preview displays the active shader pair.
5. Switch between plane, cube, torus, and at least two additional primitives.
6. Press `Ctrl+Enter` to update the active shader pair and confirm the preview
   refreshes.
7. Click the update button and confirm the preview refreshes again.
8. Change one or more exposed uniform values and confirm the render preview
   updates.
9. Change `vec2`, `vec3`, `vec4`, `mat2`, `mat3`, and `mat4` uniforms when
   the active shaders expose them and confirm the preview stays responsive.
10. Drag with the left mouse button over the render preview and confirm the
   scene rotates.
11. Drag with the right mouse button over the render preview and confirm the
   scene pans.
12. Click `Reset View` and confirm the preview camera returns to its default
   framing.
13. Rearrange the editor, render, uniform, diagnostics, and shader errors panels
   using docking.
14. Trigger at least one invalid shader compile and confirm the app remains
   responsive while showing the message in the shader errors panel.
15. Review the core source files and confirm the preview, workspace, and UI
   flow contain explanatory comments for non-obvious behavior.

## Smoke Checklist

- [ ] The project configures successfully with `cmake --preset default`
- [ ] The project builds successfully with `cmake --build --preset default`
- [ ] The automated test suite passes with `ctest --preset default`
- [ ] The app launches from `build-vcpkg/Debug/shader_editor.exe`
- [ ] The shader editor, render view, uniforms, diagnostics, and shader errors panels are available
- [ ] Orbit and pan both work from the render preview with the mouse
- [ ] Extended vector and matrix uniforms can be edited without crashing the app
- [ ] The code review confirms GLM is the active math library in the preview pipeline

## Expected Outcome

- The app builds successfully from the VS Code workflow.
- Core shader editing, update triggers, primitive switching, uniform editing,
  shader error reporting, and docking all work without freezing the UI.
