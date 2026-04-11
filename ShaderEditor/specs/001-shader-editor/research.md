# Research: Multiplatform Shader Editor

## Decision: Use C++20 with CMake as the build system

**Rationale**: C++ remains the most direct fit for OpenGL, GLFW, and Dear
ImGui. CMake provides a stable cross-platform project model and integrates well
with Visual Studio Code through tasks and CMake Tools workflows without making
VS Code a platform dependency.

**Alternatives considered**:
- Rust with graphics crates: portable, but higher integration overhead for the
  intended OpenGL + ImGui stack.
- C with handcrafted build scripts: simpler toolchain surface, but weaker
  maintainability for editor state and rendering abstractions.

## Decision: Use GLFW for windowing and OpenGL context management

**Rationale**: GLFW is lightweight, stable, and widely used for cross-platform
desktop OpenGL applications. It covers window creation, input, timing, and
context management without forcing a larger application framework.

**Alternatives considered**:
- SDL2: strong option, but broader surface area than needed for this editor.
- Native per-platform window code: rejected because it increases portability
  cost and testing overhead.

## Decision: Use Dear ImGui docking for the workspace

**Rationale**: Docking aligns directly with the requested interaction model and
  supports rapid desktop-tool composition for editor, preview, uniform, and
  status panels.

**Alternatives considered**:
- Custom panel manager: rejected due to unnecessary complexity and higher UX
  risk.
- Non-docking immediate-mode UI: rejected because it misses a stated product
  requirement.

## Decision: Keep one OpenGL context on the main application thread

**Rationale**: A single owning render thread reduces complexity around context
sharing, resource lifetime, and synchronization. Responsiveness concerns are
better handled by keeping file I/O and expensive compilation work bounded and by
surfacing clear status/error states.

**Alternatives considered**:
- Multiple shared contexts: more flexible, but unnecessary for the initial
  scope.
- Background render thread with context handoff: higher complexity and platform
  risk for limited gain in the first version.

## Decision: Represent uniforms through a typed editable metadata layer

**Rationale**: A uniform panel needs to show editable controls without forcing
users to hand-code runtime values. A typed uniform definition layer allows the
app to map scalar, vector, integer, boolean, and color-like inputs to suitable
controls and apply them consistently to the active shader preview.

**Alternatives considered**:
- Raw text entry for all uniforms: simpler to implement, but poor usability and
  error-prone for iterative preview work.
- No uniform editing in v1: rejected because the user explicitly requested it.

## Decision: Support built-in preview primitives via reusable mesh definitions

**Rationale**: Plane, cube, torus, and additional primitives can be exposed as
prebuilt meshes with shared camera/material handling. This keeps preview logic
predictable and makes model switching fast.

**Alternatives considered**:
- Loading external models only: rejected because the feature requires built-in
  primitives.
- Generating all geometry procedurally each frame: unnecessary runtime overhead.

## Decision: Use Catch2 for unit tests plus manual render smoke tests

**Rationale**: Rendering and UI behavior are partly visual, so a minimal hybrid
verification model is appropriate. Catch2 covers parser/state logic, while
manual smoke scripts validate shader compilation, docking, primitive switching,
uniform editing, and error handling.

**Alternatives considered**:
- Full UI automation first: high setup cost for early project stages.
- Manual testing only: rejected because the constitution requires a clear
  verification method and the plan benefits from automated coverage where
  practical.
