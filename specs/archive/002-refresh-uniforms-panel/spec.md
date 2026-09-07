# Feature Specification: Live Uniform Panel Refresh on Shader Recompile

**Branch**: `main`
**Created**: 2026-09-07
**Status**: Complete
**Input**: User description: "Refresh the Uniforms panel automatically whenever the shader is recompiled, so it always lists the uniforms currently declared in the active shader pair and lets the user edit their values from the panel"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Uniform list stays in sync after recompiling (Priority: P1)

A user editing the vertex or fragment shader adds, removes, or renames a uniform
and recompiles the shader pair (via `Ctrl+Enter` or the `Update Shader` button).
The `Uniforms` panel must immediately reflect the uniform set declared in the
newly compiled shader source, without requiring the user to reload the file or
restart the application.

**Why this priority**: This is the core problem reported: today the panel is
only rebuilt when a shader file is loaded, not when it is recompiled after an
edit, so the panel can show stale, missing, or renamed uniforms after normal
edit/compile iteration. Without this, uniform editing is unreliable during the
primary shader-authoring workflow.

**Independent Test**: Load a shader pair, edit the fragment shader to add a new
`uniform float uGlow;`, remove an existing uniform, and rename another. Trigger
recompilation. Verify the panel shows exactly the uniforms now declared in the
source (new one present, removed one gone, renamed one shown under its new
name) and no others.

**Acceptance Scenarios**:

1. **Given** a compiled shader pair with uniforms `uColor` and `uIntensity`
   shown in the panel, **When** the user adds `uniform vec2 uOffset;` to the
   fragment shader and recompiles, **Then** the panel shows `uColor`,
   `uIntensity`, and `uOffset`.
2. **Given** a compiled shader pair with uniform `uScale` shown in the panel,
   **When** the user deletes the `uScale` declaration from the shader source
   and recompiles, **Then** the panel no longer shows `uScale`.
3. **Given** a compiled shader pair with uniform `uColor` shown in the panel,
   **When** the user renames the declaration to `uTint` and recompiles,
   **Then** the panel shows `uTint` and no longer shows `uColor`.
4. **Given** a uniform `uColor` whose value was edited by the user to a custom
   color, **When** the user recompiles the shader without changing the
   `uColor` declaration, **Then** the panel keeps showing the user's custom
   value for `uColor` rather than resetting it to the shader's default.

---

### User Story 2 - Editing uniform values from the panel updates the live preview (Priority: P1)

A user wants to tweak the values of the uniforms currently used by the active
shader directly from the `Uniforms` panel and see the render preview update
accordingly, for every supported uniform type (`float`, `int`, `bool`, `vec2`,
`vec3`, `vec4`, `mat2`, `mat3`, `mat4`).

**Why this priority**: Editing uniform values is the primary purpose of the
panel; if a newly discovered uniform cannot be edited immediately after a
recompile, the refreshed list from User Story 1 has no practical value.

**Independent Test**: With a shader pair loaded exposing at least one uniform
of each supported type, change each uniform's value from the panel and confirm
the render preview updates to reflect the new value without further action.

**Acceptance Scenarios**:

1. **Given** the panel shows a `float` uniform `uIntensity` at its default
   value, **When** the user edits the value in the panel, **Then** the render
   preview updates using the new value on the next rendered frame.
2. **Given** a shader recompile has just added a new editable uniform,
   **When** the user immediately edits that uniform's value in the panel,
   **Then** the edit is accepted and reflected in the render preview.
3. **Given** the panel shows uniforms of type `vec2`, `vec3`, `vec4`, `mat2`,
   `mat3`, and `mat4`, **When** the user edits any of their components,
   **Then** the render preview updates using the new value.

---

### User Story 3 - Invalid recompiles do not corrupt the uniform panel (Priority: P2)

A user introduces a syntax error or otherwise breaks shader compilation. The
`Uniforms` panel must not be cleared or corrupted by the failed compile; it
should continue reflecting the last successfully compiled shader's uniforms so
editing can continue once the error is fixed.

**Why this priority**: Compile errors are a normal, frequent part of the
edit/compile loop described in the existing `Shader Errors` panel workflow.
Losing all uniform editing ability on every typo would make the panel
unusable during active editing.

**Independent Test**: With a shader pair compiled successfully and uniforms
shown in the panel, introduce a GLSL syntax error and recompile. Verify the
`Shader Errors` panel reports the failure while the `Uniforms` panel keeps
showing the uniforms and values from the last successful compile.

**Acceptance Scenarios**:

1. **Given** a successfully compiled shader with uniforms shown in the panel,
   **When** the user introduces a compile error and recompiles, **Then** the
   panel still shows the previously discovered uniforms and their values
   unchanged.
2. **Given** the state in the previous scenario, **When** the user fixes the
   error and recompiles successfully, **Then** the panel refreshes to match
   the newly compiled shader's uniform declarations per User Story 1.

---

### Edge Cases

- What happens when a recompile changes a uniform's declared type (e.g.
  `float uValue` becomes `vec3 uValue`) while keeping the same name? The panel
  must treat it as a new uniform definition (using the shader's default for
  that type) rather than reusing the old value in an incompatible type.
- What happens when the active document has no uniforms declared at all after
  a recompile (e.g. all uniforms removed)? The panel must show an empty
  uniform list without error.
- What happens when recompilation happens while the user has an in-progress
  edit (e.g. dragging a slider or typing in a numeric field) for a uniform
  that still exists after the recompile? The in-progress/current value must
  be preserved rather than reset to the shader's declared default.
- What happens on rapid, repeated recompiles (e.g. holding `Ctrl+Enter`)? The
  panel must remain responsive and consistent with the most recent successful
  compile, without flicker, duplicate entries, or crashes.
- Could this feature break the OpenGL rendering lifecycle or leave the main
  UI unresponsive during normal use? Uniform discovery and panel refresh must
  stay on the main thread's existing recompile path and complete within a
  single frame budget so the preview loop is not stalled.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST re-discover the full set of editable uniforms from
  the active shader pair's source every time a shader recompile succeeds,
  regardless of whether the recompile was triggered by `Ctrl+Enter`, the
  `Update Shader` button, or a primitive change that forces a rebuild.
- **FR-002**: System MUST update the `Uniforms` panel to show exactly the
  uniforms discovered after a successful recompile: uniforms newly declared
  MUST appear, uniforms no longer declared MUST disappear, and renamed
  uniforms MUST appear under their new name only.
- **FR-003**: System MUST preserve the current user-edited value of any
  uniform that keeps the same name and the same declared type across a
  recompile, instead of resetting it to the shader's default value.
- **FR-004**: System MUST reset a uniform to its shader-declared default value
  when a recompile changes its declared type, even if the name is unchanged.
- **FR-005**: System MUST NOT alter the uniforms shown in the panel, or their
  current values, when a recompile attempt fails to compile or link.
- **FR-006**: Users MUST be able to edit the value of every uniform currently
  listed in the panel, for all supported types (`float`, `int`, `bool`,
  `vec2`, `vec3`, `vec4`, `mat2`, `mat3`, `mat4`).
- **FR-007**: System MUST apply a uniform value edited from the panel to the
  render preview so the change is visible without requiring a manual
  recompile.
- **FR-008**: System MUST allow editing a uniform's value immediately after it
  is discovered by a recompile, with no additional user action required to
  "activate" the new uniform.
- **FR-009**: System MUST report the count of discovered uniforms in
  diagnostics after each successful recompile, consistent with existing
  diagnostics behavior for shader loads.
- **FR-010**: System MUST work identically for the Debug and Release desktop
  builds already produced by this project.
- **FR-011**: System MUST preserve existing UI patterns of the `Uniforms`
  panel (per-type controls, panel docking) unless this spec defines an
  approved change.
- **FR-012**: System MUST avoid breaking the OpenGL rendering lifecycle or
  leaving the main UI unresponsive while re-discovering uniforms and
  refreshing the panel during normal use.

### Key Entities *(include if feature involves data)*

- **UniformDefinition**: Represents one editable uniform discovered from
  shader source: name, kind/type, component count, default value, current
  value, editability, and validation rule. Recompiles replace the discovered
  set while carrying forward compatible current values.
- **UniformState**: Holds the live set of `UniformDefinition`s exposed to the
  UI for the active shader pair, and applies value edits originating from the
  panel.
- **RenderSession**: Reflects the outcome (success/error) of the most recent
  recompile attempt; a failed session must not trigger a uniform panel
  refresh.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: After any successful shader recompile, the `Uniforms` panel
  matches the shader's declared uniforms with zero manual refresh actions
  required by the user.
- **SC-002**: A user-edited uniform value that remains valid across a
  recompile (same name, same type) is never silently reset; automated tests
  verify this holds across at least one recompile per supported uniform type.
- **SC-003**: 100% of failed recompiles (compile or link errors) leave the
  panel's uniform list and values unchanged from before the failed attempt.
- **SC-004**: Editing any uniform value from the panel updates the render
  preview within the next rendered frame, for all nine supported uniform
  types.
- **SC-005**: The primary edit/recompile/edit-uniform workflow completes on
  each supported desktop platform with no blocking errors.
- **SC-006**: The app keeps a valid render loop and remains responsive during
  repeated rapid recompiles triggered in quick succession.

## Assumptions

- The existing `UniformIntrospectionService::discover` parsing logic already
  correctly extracts uniform declarations from GLSL source; this feature only
  changes when discovery is invoked and how discovered results are merged
  with existing user-edited values, not the parsing rules themselves.
- "Recompile" refers to any call into the existing `WorkspaceController`
  update path (`updateShaders`), whether triggered by `Ctrl+Enter`, the
  `Update Shader` button, or a primitive switch that forces a shader rebuild.
- Matching a uniform across a recompile for value-preservation purposes is
  done by name and declared type together, consistent with `UniformDefinition`
  fields already defined in the codebase.
- No new persistence is required: uniform values remain in-memory for the
  current session only, consistent with current behavior.
- Multi-shader-pair or multi-document workflows are out of scope; this
  feature targets the single active shader pair already supported by
  `WorkspaceController`.
