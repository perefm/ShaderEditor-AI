# ShaderEditor

ShaderEditor is a desktop OpenGL 4.6 shader workspace for editing Phoenix-style
single-file GLSL shaders, previewing them on built-in 3D primitives, and arranging the
tool panels in a dockable Dear ImGui layout.

## Features

- Load, edit, and save Phoenix `.glsl` files containing `#type vertex` and `#type fragment`
- Live preview on plane, cube, torus, sphere, and cylinder
- Dockable editor, render, uniforms, diagnostics, and shader error panels
- `Ctrl+Enter` and button-driven shader recompilation
- Editable runtime uniforms including `float`, `int`, `bool`, `vec2`, `vec3`,
  `vec4`, `mat2`, `mat3`, `mat4`, and `sampler2D`
- Mouse-driven preview navigation
  - Left drag: orbit the scene
  - Right drag: pan the scene
  - `Reset View`: restore the default framing
- GLM-based math pipeline for preview transforms and uniform upload
- Phoenix-compatible implicit shader uniforms: `MVP` and `uCameraPos`

### Engine-provided uniforms

The preview engine supplies these Phoenix-compatible uniforms automatically on
every rendered frame. They must not be declared as editable uniforms in the
`Uniforms` panel:

- `uniform mat4 MVP`: model-view-projection matrix for the selected preview
  primitive.
- `uniform vec3 uCameraPos`: camera position in preview world space, updated
  when orbit, pan, or zoom changes.

Declare and use these names in shader stages that need them. Other uniforms
declared by the shader are discovered after a successful compile and remain
editable from the `Uniforms` panel.

## Project Layout

```text
src/      Application, workspace, rendering, and UI code
assets/   Bundled GLSL shader assets
tests/    Catch2 unit tests
specs/    Archived Spec Kit feature artifacts
```

## Dependencies

- CMake 3.27+
- A C++20 compiler
- OpenGL-capable desktop environment
- vcpkg with the following manifest dependencies:
  - `glad`
  - `glfw3`
  - `imgui[docking-experimental,glfw-binding,opengl3-binding]`
  - `glm`
  - `stb`

### Install vcpkg

If vcpkg is not already installed, clone it and bootstrap it once:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\tools\vcpkg
& C:\tools\vcpkg\bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT` in each PowerShell session before configuring the project.
Use the existing installation path if vcpkg is already installed:

```powershell
$env:VCPKG_ROOT = "C:\tools\vcpkg"
```

The CMake preset uses this variable to locate the vcpkg toolchain. Dependencies
declared in `vcpkg.json` are installed automatically during configuration.

## Build

### Configure

```powershell
$env:VCPKG_ROOT = "C:\tools\vcpkg"
cmake --preset default
```

Run the command from the repository root. If CMake reports that the vcpkg
toolchain cannot be found, verify that `VCPKG_ROOT` points to the directory
containing `scripts\buildsystems\vcpkg.cmake`, then configure again.

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

### Visual Studio Code

Set `VCPKG_ROOT` before starting Visual Studio Code so the bundled CMake tasks
and the CMake Tools extension inherit it:

```powershell
$env:VCPKG_ROOT = "C:\tools\vcpkg"
code .
```

Then select the `default` configure preset and run `cmake-build` or `ctest`
from the Command Palette. The `Run Shader Editor` launch configuration builds
the Debug target before starting the application.

## How To Use

1. Launch the application.
2. Load the bundled example shaders or open a local Phoenix `.glsl` file.
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
[`specs/archive/001-shader-editor/quickstart.md`](specs/archive/001-shader-editor/quickstart.md).
