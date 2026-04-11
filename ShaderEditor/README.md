# ShaderEditor

ShaderEditor is a desktop OpenGL shader workspace for editing paired vertex and
fragment shaders, previewing them on built-in 3D primitives, and arranging the
tool panels in a dockable Dear ImGui layout.

## Features

- Load, edit, and save vertex and fragment shader files in one workspace
- Live preview on plane, cube, torus, sphere, and cylinder
- Dockable editor, render, uniforms, diagnostics, and shader error panels
- `Ctrl+Enter` and button-driven shader recompilation
- Editable runtime uniforms including `float`, `int`, `bool`, `vec2`, `vec3`,
  `vec4`, `mat2`, `mat3`, and `mat4`
- Mouse-driven preview navigation
  - Left drag: orbit the scene
  - Right drag: pan the scene
  - `Reset View`: restore the default framing
- GLM-based math pipeline for preview transforms and uniform upload

## Project Layout

```text
src/      Application, workspace, rendering, and UI code
assets/   Sample shader assets and primitive placeholders
tests/    Catch2 unit tests
specs/    Spec Kit feature artifacts
```

## Dependencies

- CMake 3.27+
- A C++20 compiler
- OpenGL-capable desktop environment
- vcpkg dependencies:
  - `glad`
  - `glfw3`
  - `imgui[docking-experimental,glfw-binding,opengl3-binding]`
  - `glm`

## Build

### Configure

```powershell
cmake --preset default
```

### Debug

```powershell
cmake --build --preset default
ctest --preset default
```

Launch:

```powershell
build-vcpkg\Debug\shader_editor.exe
```

### Release

```powershell
cmake --build build-vcpkg --config Release
ctest --test-dir build-vcpkg -C Release --output-on-failure
```

Launch:

```powershell
build-vcpkg\Release\shader_editor.exe
```

## How To Use

1. Launch the application.
2. Load the bundled example shaders or open local vertex/fragment shader files.
3. Edit shader source in the `Shader Editor` panel.
4. Recompile with `Ctrl+Enter` or `Update Shader`.
5. Switch the preview primitive in `Render View`.
6. Drag with the mouse over the preview:
   - Left button: orbit
   - Right button: pan
7. Edit uniforms in the `Uniforms` panel.
8. Check `Diagnostics` and `Shader Errors` when file, compile, or render issues occur.
9. Rearrange the dockable panels to fit the current workflow.

## Current Validation

The automated validation flow currently covers:

- `cmake --build --preset default`
- `ctest --preset default`
- `cmake --build build-vcpkg --config Release`
- `ctest --test-dir build-vcpkg -C Release --output-on-failure`

Manual validation steps are documented in
[`specs/001-shader-editor/quickstart.md`](specs/001-shader-editor/quickstart.md).
