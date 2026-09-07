# Implementation Plan: Multiplatform Shader Editor

**Branch**: `main` | **Date**: 2026-04-11 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/archive/001-shader-editor/spec.md`

## Summary

Build a multiplatform desktop shader editor for paired vertex and fragment
OpenGL shaders with an integrated dockable workspace, live render preview on
multiple primitives, and editable shader uniforms. The implementation will use
C++20, CMake, OpenGL, GLFW, and Dear ImGui, with a VS Code-friendly build and
debug workflow that remains portable across supported desktop platforms.

## Technical Context

**Language/Version**: C++20  
**Primary Dependencies**: OpenGL, GLFW, Dear ImGui, GLM, glad, native file dialogs  
**Storage**: Local filesystem for shader source files and optional workspace settings  
**Testing**: Catch2 for non-rendering unit tests plus manual render smoke validation  
**Target Platform**: Windows, Linux, and macOS desktop builds with OpenGL-capable GPUs  
**Project Type**: Desktop application  
**Performance Goals**: UI input and docking remain responsive; model switch and uniform updates reflected in preview within 1 second for normal shaders  
**Constraints**: Must build from VS Code using CMake tasks, remain multiplatform, preserve a stable OpenGL context lifecycle, and avoid blocking the main UI during normal editing workflows  
**Scale/Scope**: Single-user local desktop tool with one active vertex/fragment shader pair, dockable windows, built-in preview primitives, and editable uniform controls

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- Platform compatibility: Pass. CMake + GLFW keeps the build portable across
  Windows, Linux, and macOS, while file handling is isolated behind app-level
  services.
- Verification: Pass. Plan includes Catch2 unit tests for parsing/state logic
  and manual smoke validation for shader compilation, render preview, docking,
  and cross-platform launch.
- UI consistency: Pass. Major surfaces are limited to dockable editor, render,
  uniforms, and status/error panels under one consistent ImGui workspace model.
- OpenGL/runtime impact: Pass. Rendering design isolates context ownership,
  shader compilation, mesh preview switching, and uniform updates behind a
  dedicated render subsystem with explicit error propagation.

## Project Structure

### Documentation (this feature)

```text
specs/archive/001-shader-editor/
|-- plan.md
|-- research.md
|-- data-model.md
|-- quickstart.md
|-- contracts/
|   `-- ui-workspace.md
`-- tasks.md
```

### Source Code (repository root)

```text
src/
|-- app/
|   |-- application/
|   |-- workspace/
|   `-- platform/
|-- rendering/
|   |-- opengl/
|   |-- shaders/
|   `-- geometry/
|-- editor/
|-- ui/
|   |-- dockspace/
|   |-- panels/
|   `-- widgets/
`-- services/
    |-- files/
    `-- persistence/

assets/
|-- primitives/
`-- shaders/

tests/
|-- unit/
`-- integration/

.vscode/
|-- tasks.json
|-- launch.json
`-- settings.json
```

**Structure Decision**: Use a single CMake-based desktop application with clear
separation between application state, rendering, UI panels, and file services.
VS Code workspace configuration lives alongside the app to support local build,
run, and debug flows without coupling the implementation to one operating
system.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| None | N/A | N/A |
