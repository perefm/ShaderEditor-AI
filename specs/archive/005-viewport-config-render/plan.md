# Implementation Plan: Viewport Phoenix Variables, Configuration Panel, and Render Feedback

**Branch**: `005-viewport-config-render` | **Date**: 2026-09-08 | **Spec**: [spec.md](./spec.md)  
**Input**: Feature specification from `/specs/005-viewport-config-render/spec.md`

**Phoenix Reference Commit**: `Spontz/Phoenix` @
[`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`](https://github.com/Spontz/Phoenix/commit/75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d)
(tag `v4.2.4`, 2026-08-27). The relevant reference is
`Engine/src/core/drivers/MathDriver.cpp`, where Phoenix registers
`vpWidth`, `vpHeight`, and `aspectRatio` from the current viewport.

## Summary

Add four focused improvements to the existing C++20/OpenGL desktop app:

1. Extend Phoenix auto-uniform support with `vpWidth`, `vpHeight`, and
   `aspectRatio`, derived from the Render panel's effective preview viewport and
   uploaded every frame when declared as `float` uniforms.
2. Add a dockable "Config" panel for editor text size and runtime vsync
   toggling.
3. Show a smoothed FPS readout in the Render panel.
4. Let users choose the Render preview background/clear color from the Render
   panel.

The implementation should reuse the existing ImGui panel pattern,
`WorkspaceController` render state flow, `PreviewRenderer` offscreen render path,
and `UniformProvenance::PhoenixAuto` model introduced by the earlier Phoenix
auto-uniform work.

## Final Implementation Status

Completed on 2026-09-08. The implementation includes:

- `vpWidth`, `vpHeight`, and `aspectRatio` Phoenix auto-uniform discovery and
  per-frame upload from the Render preview viewport.
- A dockable "Config" panel with editor text-size scaling and runtime
  VSync toggling.
- Persisted editor text scale, requested VSync state, and Render background
  color in `shader_editor_settings.ini` beside the executable.
- Render-panel FPS display and background color picker.
- README and in-app Shader Help updates for the new Phoenix viewport uniforms.
- Targeted unit tests for viewport auto-uniform introspection, viewport metrics,
  and FPS stability.

## Technical Context

**Language/Version**: C++20  
**Primary Dependencies**: OpenGL 4.6 core, GLFW, Dear ImGui docking, GLM, glad,
Assimp, stb_image, `imgui_color_text_edit`  
**Storage**: Local preference file beside the executable if no existing
preference mechanism is available; existing `imgui.ini` remains dedicated to
ImGui layout/docking  
**Testing**: Existing CTest target `shader_editor_tests`, with targeted unit
coverage added for non-GL logic and manual verification for live ImGui/OpenGL
behavior  
**Target Platform**: Windows desktop, matching the current GLFW/OpenGL and
Win32-dialog application path  
**Project Type**: Single desktop application (`src/`, `tests/`)  
**Performance Goals**: Maintain responsive UI and valid frame pacing with vsync
enabled or disabled; FPS display should update at human-readable cadence rather
than every raw frame  
**Constraints**: Do not recreate the window/OpenGL context for vsync or color
changes; do not expose Phoenix auto-uniforms as editable generic uniforms; do
not reset shader/model/camera/playback state when changing configuration or
background color  
**Scale/Scope**: One active shader document, one active render target, one
application window; no multi-window settings or per-project profiles

## Constitution Check

*GATE: Must pass before implementation. Re-check after design.*

- **Platform compatibility**: Pass. Feature targets the current Windows desktop
  build path and uses GLFW APIs already present in the application. Preference
  persistence must use platform-safe filesystem paths.
- **Verification**: Pass. Add targeted unit tests for uniform introspection,
  viewport metric math, FPS smoothing, and settings serialization if introduced.
  Manual verification covers live vsync, editor text size, clear color, and
  visible FPS behavior.
- **UI consistency**: Pass. The "Config" panel follows existing
  dockable/toggleable panel visibility conventions; Render controls are added
  alongside current Render panel controls.
- **OpenGL/runtime impact**: Pass with caution. `glfwSwapInterval` must be
  called against the active context at runtime, and clear color changes must
  flow into the existing offscreen preview clear path used by primitives and
  imported models.

No constitution violations are expected.

## Project Structure

### Documentation (this feature)

```text
specs/005-viewport-config-render/
├── spec.md
├── plan.md
├── research.md          # Optional if implementation discovers additional decisions
├── data-model.md        # Optional if preference/render metric types expand
├── quickstart.md        # Optional manual validation checklist
└── tasks.md             # Created by the tasks step, not by this plan
```

### Source Code (repository root)

```text
src/
├── app/
│   ├── application/
│   │   ├── Application.h        # panel visibility, editor text scale, settings object, UI draw hook
│   │   └── Application.cpp      # "Config" panel, Render panel FPS/color controls, help text
│   ├── platform/
│   │   ├── WindowContext.h      # vsync setter/requested state facade
│   │   └── WindowContext.cpp    # glfwSwapInterval application
│   └── workspace/
│       ├── WorkspaceController.h
│       └── WorkspaceController.cpp # render preview state, background color/metrics propagation
├── rendering/
│   ├── opengl/
│   │   ├── PreviewRenderer.h    # configurable clear color
│   │   └── PreviewRenderer.cpp  # viewport auto-uniform upload + clear color use
│   └── shaders/
│       ├── RenderSession.h      # viewport metrics, FPS, background color snapshot if needed
│       └── UniformIntrospectionService.cpp # recognize viewport Phoenix auto-uniforms
└── ui/
    └── panels/
        ├── RenderViewPanel.h/.cpp # pass-throughs if controls remain thin
        └── ShaderEditorPanel.h/.cpp # no behavior change expected

tests/
├── CMakeLists.txt
└── unit/
    ├── test_uniform_introspection.cpp
    ├── test_render_session.cpp
    └── test_app_settings.cpp      # new only if settings serialization is extracted/testable
```

**Structure Decision**: Keep the feature in the existing app/workspace/rendering
layers. Add small data helpers only where they improve testability; avoid a new
subsystem unless preference persistence cannot be kept simple inside the current
application layer.

## Phase 0: Research and Decisions

1. **Phoenix viewport semantics**
   - Decision: `vpWidth`, `vpHeight`, and `aspectRatio` are exact names.
   - Decision: values come from the effective Render preview viewport, not the
     whole application framebuffer.
   - Decision: ShaderEditor exposes them to GLSL as `float` auto-uniforms.

2. **Settings persistence**
   - Decision: `imgui.ini` remains for ImGui layout only.
   - Decision: if no existing preference store exists, add a minimal
     application settings file beside the executable for editor text scale,
     vsync, and background color.
   - Constraint: settings load failures should be reported consistently through
     diagnostics or fall back to defaults with an explicit info/warning entry,
     not silently hidden.

3. **Editor text size**
   - Decision: prefer changing only the `TextEditor` control's scale/style if
     supported.
   - Fallback: if the vendored editor cannot scale independently, use the
     narrowest ImGui font-scale mechanism around the shader editor draw call and
     document the limitation in implementation notes.

4. **FPS measurement**
   - Decision: derive FPS from frame timestamps already available in the render
     loop or `WorkspaceController::renderPreview`.
   - Decision: smooth using a short rolling/accumulated interval (for example
     0.25-1.0 seconds) so text remains readable.

5. **OpenGL clear color**
   - Decision: route selected Render background color into `PreviewRenderer`
     and use it in both primitive and model clear paths.

## Phase 1: Design

### Data Model

- **ViewportAutoUniforms**
  - `widthPixels: int`
  - `heightPixels: int`
  - `aspectRatio: float`
  - Invariant: width and height are clamped to positive values before computing
    aspect ratio.

- **ApplicationSettings**
  - `editorTextScale: float`
  - `vsyncEnabled: bool`
  - `renderBackgroundColor: glm::vec4` or equivalent RGBA struct
  - Defaults: text scale `1.0`, vsync enabled, current preview clear color
    `(0.09, 0.10, 0.13, 1.0)`.
  - Bounds: text scale must remain in a usable range, e.g. `0.75` to `2.0`.

- **RenderMetrics**
  - `lastFrameTime`
  - accumulated frame count/time for smoothing
  - `displayFps: float`
  - Invariant: displayed FPS is finite and non-negative.

### Service/API Contracts

- `WindowContext`
  - Add `setVsyncEnabled(bool enabled)` that calls `glfwSwapInterval(enabled ? 1 : 0)`.
  - Add `vsyncEnabled()` or requested-state accessor for UI consistency.

- `WorkspaceController`
  - Add background color setters/getters or include color in render session.
  - Ensure `renderPreview(width, height)` publishes current viewport dimensions
    before the renderer uploads auto-uniforms.
  - Optionally own `RenderMetrics` if FPS should track Render panel preview
    frames specifically.

- `PreviewRenderer`
  - Accept/store preview clear color.
  - Upload `vpWidth`, `vpHeight`, and `aspectRatio` in `applyUniforms` for
    `UniformProvenance::PhoenixAuto` float definitions.
  - Use the selected clear color in primitive and model frame setup.

- `UniformIntrospectionService`
  - Extend `isPhoenixAutoUniform` with float uniforms named `vpWidth`,
    `vpHeight`, and `aspectRatio`.

- `Application`
  - Register and draw "Config".
  - Draw editor text-size and vsync controls.
  - Draw Render panel FPS text and background color picker.
  - Load/save app settings during initialize/shutdown or on change.

## Phase 2: Implementation Approach

1. Add/test viewport auto-uniform discovery:
   - Update `UniformIntrospectionService`.
   - Extend `test_uniform_introspection.cpp` for the three new names and for an
     incompatible type remaining user-managed or diagnostic-consistent.

2. Add viewport values to render flow:
   - Add fields to `RenderSession` or pass explicit viewport dimensions into
     `applyUniforms`.
   - Upload values after the current program is active and before draw calls.
   - Add unit coverage for any pure viewport/aspect helper.

3. Add configurable background color:
   - Replace hard-coded preview `glClearColor(0.09F, 0.10F, 0.13F, 1.0F)` in
     both primitive and model paths with configured color.
   - Add Render panel color picker and route changes without recompiling or
     resetting render state.

4. Add FPS display:
   - Compute smoothed FPS from actual frame timing.
   - Display in Render panel near existing playback/preview controls.
   - Guard against zero/negative delta and non-finite values.

5. Add "Config" panel:
   - Add panel visibility flag and registration.
   - Add menu toggle using existing View/menu pattern.
   - Add bounded editor text-size control plus reset.
   - Add vsync checkbox calling `WindowContext::setVsyncEnabled`.

6. Add preference persistence:
   - Use a small settings type and file if no existing preference store is
     available.
   - Persist editor text scale, requested vsync state, and background color.
   - Load before first UI frame and save on shutdown or on each change.

7. Update documentation/help:
   - Add viewport variables to README's engine-provided uniforms section.
   - Add viewport variables to the in-app Shader Help text.

## Verification Plan

### Automated

Run targeted tests after implementation:

```powershell
ctest --test-dir build-vcpkg --output-on-failure -C Debug -R shader_editor_tests
```

Expected additional/updated tests:

- `test_uniform_introspection.cpp`: `vpWidth`, `vpHeight`, `aspectRatio` are
  recognized as `PhoenixAuto` only when declared as `float`.
- `test_render_session.cpp` or a new helper test: aspect ratio clamps safely and
  never returns NaN/Inf.
- `test_app_settings.cpp` if a settings parser/writer is introduced.

### Manual

1. Launch the app and confirm "Config" appears in the panel/menu flow.
2. Change editor text size; confirm only shader editor text changes and unsaved
   edits remain intact.
3. Toggle vsync off/on and confirm the FPS display reacts plausibly.
4. Change Render background color and confirm the next frame clears to the new
   color without resetting playback, camera, shader, uniforms, model, or texture
   selections.
5. Load a shader using `uniform float vpWidth;`, `uniform float vpHeight;`, and
   `uniform float aspectRatio;`; resize the Render panel and confirm the shader
   output reflects the new dimensions.
6. Close and reopen the app; confirm persisted preferences restore when the
   settings store is implemented.

## Complexity Tracking

No complexity exceptions are required. The plan stays within existing modules
and avoids new external dependencies.
