# Feature Specification: Viewport Phoenix Variables, Configuration Panel, and Render Feedback

**Feature Branch**: `005-viewport-config-render`  
**Created**: 2026-09-08  
**Status**: Implemented  
**Input**: User description: "Se han de añadir variables como `vpWidth`, `vpHeight` y `aspectRatio`, tal y como define Phoenix. Se ha de crear un nuevo panel con \"Config\" que permita cambiar el tamaño del texto del editor y permita habilitar o deshabilitar el vsync. En el panel de render se han de indicar los FPS's a los que se está renderizando. En el panel de render se ha de poder cambiar el color del fondo."

**Phoenix Reference Commit**: The viewport variable names and data source are
derived from the `Spontz/Phoenix` GitHub repository at commit
[`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`](https://github.com/Spontz/Phoenix/commit/75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d)
(tag `v4.2.4`, 2026-08-27), specifically
`Engine/src/core/drivers/MathDriver.cpp`, where Phoenix registers:
`vpWidth`, `vpHeight`, and `aspectRatio` from the current viewport's
`Width`, `Height`, and `AspectRatio`.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Preview shaders that depend on Phoenix viewport variables (Priority: P1)

As a shader author targeting Phoenix, I want ShaderEditor to automatically
provide `vpWidth`, `vpHeight`, and `aspectRatio`, so shaders that use Phoenix's
viewport constants render correctly without adding manual uniform controls.

**Why this priority**: These variables are part of Phoenix compatibility and
directly affect whether existing shaders can be previewed accurately.

**Independent Test**: Load a Phoenix `.glsl` shader declaring `uniform float
vpWidth;`, `uniform float vpHeight;`, and `uniform float aspectRatio;`, render
it in the Render panel, resize the panel, and verify the visible output changes
according to the current preview dimensions and ratio.

**Acceptance Scenarios**:

1. **Given** a shader declares `uniform float vpWidth;`, **When** the Render
   preview is visible, **Then** the value uploaded to the shader equals the
   current preview viewport width in pixels.
2. **Given** a shader declares `uniform float vpHeight;`, **When** the Render
   preview is visible, **Then** the value uploaded to the shader equals the
   current preview viewport height in pixels.
3. **Given** a shader declares `uniform float aspectRatio;`, **When** the Render
   preview is visible, **Then** the value uploaded to the shader equals
   `vpWidth / vpHeight` for the effective preview viewport.
4. **Given** the user resizes the Render panel or window, **When** the next
   frame renders, **Then** `vpWidth`, `vpHeight`, and `aspectRatio` reflect the
   new preview dimensions without requiring shader recompilation.
5. **Given** a shader does not declare one or more of these variables, **When**
   it renders, **Then** no warning or error is produced for the omitted
   variables.

---

### User Story 2 - Configure editor readability and vsync from a dedicated panel (Priority: P1)

As a user, I want a dedicated "Config" panel where I can change the
shader editor text size and toggle vsync, so I can tune the editing experience
and rendering cadence without restarting or editing code.

**Why this priority**: Text size and vsync are user-facing usability controls
and need a clear location separate from shader content and render controls.

**Independent Test**: Open the "Config" panel, adjust the editor text
size, toggle vsync off and on, and verify both changes apply immediately while
the current shader and render preview remain active.

**Acceptance Scenarios**:

1. **Given** the application is running, **When** the user opens the View menu
   or equivalent panel selector, **Then** a panel named "Config" can be
   shown or hidden using the existing panel visibility pattern.
2. **Given** the "Config" panel is open, **When** the user changes the
   editor text size, **Then** the shader editor text updates immediately and
   remains usable without losing cursor position, scroll state, or unsaved
   edits.
3. **Given** the "Config" panel is open, **When** the user disables
   vsync, **Then** the application applies the change to the active window swap
   interval and the render loop is no longer capped by vertical sync.
4. **Given** vsync is disabled, **When** the user enables it again, **Then** the
   application applies the active window swap interval used for vsync-enabled
   rendering.
5. **Given** the user changes text size or vsync, **When** the application is
   closed and reopened, **Then** the last selected values are restored if the
   project has an existing settings persistence mechanism; otherwise the plan
   for this feature MUST define and implement a simple preference store.

---

### User Story 3 - See render FPS in the Render panel (Priority: P2)

As a shader author, I want the Render panel to display the FPS currently being
rendered, so I can understand the runtime performance impact of my shader,
model, vsync setting, and background color choices.

**Why this priority**: FPS is essential feedback for rendering work, but it is
secondary to Phoenix compatibility and basic configuration controls.

**Independent Test**: Open the Render panel with a shader running, observe the
FPS label, toggle vsync, and verify the displayed value updates over time and
reflects the change in frame pacing.

**Acceptance Scenarios**:

1. **Given** the Render panel is visible, **When** frames are being rendered,
   **Then** the panel displays a readable FPS value near the preview controls.
2. **Given** rendering continues for multiple seconds, **When** frame times
   fluctuate, **Then** the displayed FPS updates smoothly enough to be useful
   without flickering every individual frame.
3. **Given** the preview is unavailable or not yet backed by a render texture,
   **When** the Render panel is visible, **Then** the FPS display remains valid
   and does not show NaN, Inf, or crash the UI.
4. **Given** vsync is toggled, **When** rendering continues, **Then** the FPS
   readout updates according to the new frame pacing.

---

### User Story 4 - Change the Render panel background color (Priority: P2)

As a shader author, I want to change the render background color from the Render
panel, so transparent, additive, dark, or bright shaders can be evaluated
against a suitable backdrop.

**Why this priority**: Background color improves visual inspection and is
directly tied to the render preview, but it does not block shader execution.

**Independent Test**: Open the Render panel, choose a background color, render a
shader that does not cover every pixel or uses transparency, and verify the
preview clears to the selected color immediately and consistently.

**Acceptance Scenarios**:

1. **Given** the Render panel is visible, **When** the user changes the
   background color, **Then** the next rendered frame uses that color as the
   preview clear color.
2. **Given** the user changes the background color while a shader or imported
   model is active, **When** rendering continues, **Then** the active shader,
   model, camera, uniforms, and playback state are preserved.
3. **Given** the selected background color includes alpha, **When** the preview
   is rendered to the application's offscreen texture, **Then** the visible
   preview uses the selected RGB color and handles alpha consistently with the
   existing preview pipeline.
4. **Given** the application is closed and reopened, **When** the Render panel
   is shown, **Then** the last selected background color is restored if settings
   persistence is available for this feature.

### Edge Cases

- What happens when the Render panel is collapsed or has near-zero available
  space? The effective viewport values MUST stay clamped to valid positive
  dimensions, and `aspectRatio` MUST never become NaN or Inf.
- What happens when the editor text size is set too small or too large? The
  control MUST enforce a usable bounded range and expose a default/reset path.
- What happens when the platform or driver does not honor vsync changes? The UI
  MUST keep the requested value visible and surface any available diagnostic
  using the existing application diagnostics pattern rather than silently
  failing.
- What happens when vsync is disabled and rendering becomes much faster than the
  display refresh rate? The UI MUST remain responsive and FPS calculation MUST
  remain numerically stable.
- What happens when a shader declares `vpWidth`, `vpHeight`, or `aspectRatio`
  with an incompatible GLSL type? The system MUST treat it like existing
  auto-uniform type mismatches: no crash, no forced upload, and a clear
  diagnostic if the existing uniform flow supports one.
- Could this feature break OpenGL context setup, shader/resource loading, or UI
  responsiveness during normal use? Changing vsync and clear color MUST operate
  on the active context/render path without recreating the window or invalidating
  loaded resources.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST recognize `vpWidth`, `vpHeight`, and
  `aspectRatio` as Phoenix-compatible auto-supplied uniforms when a shader
  declares them as scalar `float` uniforms.
- **FR-002**: `vpWidth` MUST equal the effective Render preview viewport width
  in pixels for the current frame.
- **FR-003**: `vpHeight` MUST equal the effective Render preview viewport height
  in pixels for the current frame.
- **FR-004**: `aspectRatio` MUST equal `vpWidth / vpHeight` for the current
  frame, using a positive non-zero height.
- **FR-005**: The viewport auto-uniform values MUST update every rendered frame
  and after Render panel/window resizing without requiring shader recompilation.
- **FR-006**: The viewport auto-uniforms MUST be visually identified as
  automatically managed values and MUST NOT be editable in the generic Uniforms
  panel.
- **FR-007**: Shaders that omit any of `vpWidth`, `vpHeight`, or `aspectRatio`
  MUST compile and render without warnings related to their absence.
- **FR-008**: If a shader declares any viewport auto-uniform with an
  incompatible type, the system MUST avoid uploading the automatic value and
  MUST surface the mismatch consistently with existing uniform diagnostics.
- **FR-009**: The application MUST expose a dockable/toggleable panel named
  "Config" following the existing ImGui panel/menu conventions.
- **FR-010**: The "Config" panel MUST include a control for shader editor
  text size with bounded values, immediate application, and an easy return to
  the default size.
- **FR-011**: Changing shader editor text size MUST affect only editor text, not
  global UI labels, menus, render preview size, or diagnostic text unless the
  implementation plan explicitly justifies a broader ImGui limitation.
- **FR-012**: The "Config" panel MUST include a vsync toggle whose state
  is applied to the active GLFW/OpenGL window swap interval at runtime.
- **FR-013**: The vsync control MUST show the currently requested state and MUST
  remain consistent when the panel is hidden and shown again.
- **FR-014**: The Render panel MUST display a current FPS value while the
  application is running.
- **FR-015**: FPS MUST be computed from actual frame timing and smoothed or
  averaged over a short interval so the displayed value is readable.
- **FR-016**: The FPS display MUST never show invalid numeric values such as
  NaN, Inf, or negative FPS.
- **FR-017**: The Render panel MUST include a background color control.
- **FR-018**: The selected background color MUST be used as the OpenGL clear
  color for the offscreen Render preview before drawing the active shader/model.
- **FR-019**: Changing background color MUST not reload shaders, reset playback,
  reset camera interaction, clear selected textures, or unload imported models.
- **FR-020**: Text size, requested vsync state, and background color SHOULD be
  persisted between application launches when a project preference mechanism is
  implemented for this feature.
- **FR-021**: The system MUST update README/help text that lists
  engine-provided uniforms so it includes `vpWidth`, `vpHeight`, and
  `aspectRatio`.
- **FR-022**: System MUST work on the intended supported platforms defined for
  the feature: Windows with the existing GLFW/OpenGL application path.
- **FR-023**: System MUST preserve existing UI patterns, shader editing flows,
  uniform editing behavior, diagnostics behavior, and render interaction unless
  this spec explicitly changes them.
- **FR-024**: System MUST avoid breaking the OpenGL rendering lifecycle or
  leaving the main UI unresponsive during normal use.

### Key Entities *(include if feature involves data)*

- **ViewportAutoUniforms**: Per-frame values supplied to compatible shader
  uniforms: `vpWidth`, `vpHeight`, and `aspectRatio`. They are derived from the
  effective Render preview viewport, not from the full application window size.
- **EditorConfiguration**: User preferences for shader editor text size and
  requested vsync state. Values are controlled from the "Config" panel
  and are expected to remain stable while panels are hidden/shown.
- **RenderMetrics**: Frame timing data used to compute a readable FPS display in
  the Render panel.
- **RenderBackground**: The selected preview clear color used by the offscreen
  renderer before drawing the active scene.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A shader that colors output based on `vpWidth`, `vpHeight`, and
  `aspectRatio` visibly updates within one rendered frame after resizing the
  Render panel.
- **SC-002**: The "Config" panel can change editor text size and vsync
  state during an active render session without losing unsaved shader edits.
- **SC-003**: The Render panel displays a valid FPS value continuously during a
  30-second render run with vsync both enabled and disabled.
- **SC-004**: Changing the Render background color is visible on the next frame
  and does not reset active shader, model, texture, camera, or playback state.
- **SC-005**: Existing Phoenix auto-uniforms from earlier specs (`t`, `tend`,
  `beat`, matrices/material/model values) continue to work after adding the new
  viewport variables.
- **SC-006**: Primary manual validation completes on Windows with no blocking
  OpenGL, GLFW, or ImGui errors.

## Assumptions

- `vpWidth`, `vpHeight`, and `aspectRatio` are exposed to GLSL shaders as
  scalar `float` uniforms, matching ShaderEditor's existing auto-uniform upload
  model and Phoenix's expression-variable naming.
- The effective viewport for these variables is the offscreen Render preview
  texture size displayed inside the Render panel, not the whole OS window or the
  full ImGui viewport.
- "Config" is the visible panel/menu label for configuration controls.
- Text size refers to the shader code editor control only. If the current editor
  implementation cannot scale only editor text, the implementation plan must
  document the limitation and choose the narrowest possible UI-scaling behavior.
- Vsync defaults to enabled, preserving the current application behavior unless
  the user changes it.
- Background color defaults to the current preview clear color so existing
  shaders look unchanged until the user chooses a new color.
