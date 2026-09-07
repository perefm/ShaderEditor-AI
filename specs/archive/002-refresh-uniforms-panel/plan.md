# Implementation Plan: Live Uniform Panel Refresh on Shader Recompile

**Branch**: `main` | **Date**: 2026-09-07 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/archive/002-refresh-uniforms-panel/spec.md`

## Summary

Make the uniform state authoritative after every successful shader recompile. The workspace controller will compile the edited shader pair first, rediscover uniforms only after compilation/linking succeeds, and merge the new definitions with compatible existing values by uniform name and declared type. The Uniforms panel will consume that refreshed state and expose controls for all supported GLSL uniform types; failed recompiles will retain the last successful definitions and values.

## Technical Context

**Language/Version**: C++20  
**Primary Dependencies**: OpenGL, GLFW, Dear ImGui, GLM, glad, Catch2  
**Storage**: In-memory uniform state for the active shader pair; no new persistence  
**Testing**: Catch2 unit tests for introspection/state/recompile behavior plus manual ImGui/OpenGL smoke validation  
**Target Platform**: Existing Windows, Linux, and macOS desktop targets with OpenGL-capable GPUs  
**Project Type**: Desktop application feature  
**Performance Goals**: Uniform refresh completes as part of the existing recompile path without visible UI stalls; edits appear in the next rendered frame  
**Constraints**: Preserve existing panel/docking patterns, OpenGL context lifecycle, diagnostics, and failed-compile behavior  
**Scale/Scope**: One active vertex/fragment shader pair and the existing nine supported uniform value types

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- Platform compatibility: Pass. The feature changes shared C++ state and existing ImGui/OpenGL paths only; no platform-specific APIs are introduced.
- Verification: Pass. Add focused Catch2 coverage for compatible value retention, type changes, additions/removals, and failed recompiles; retain existing Debug/Release build and test validation.
- UI consistency: Pass. Reuse the existing `UniformState`, `UniformDefinition`, `WorkspaceController`, and dockable `UniformsPanel` patterns rather than introducing a second panel model.
- OpenGL/runtime impact: Pass with constraint. Compile/refresh ordering must commit new uniform state only after a successful render-session update; failed attempts leave the prior state intact and all edits continue through the existing preview refresh path.

## Phase 0: Research and implementation decisions

1. Trace the current recompile path from `WorkspaceController::updateShaders` through `PreviewRenderer::updatePreview` and the UI triggers (`Ctrl+Enter` and update button).
2. Confirm the existing `UniformIntrospectionService::discover` output for declarations in both shader stages and identify whether duplicate names need deterministic merging.
3. Confirm how `UniformState::setDefinitions` currently initializes or replaces `currentValue`, then define a name-and-kind compatible merge that preserves values without unsafe variant casts.
4. Confirm the panel's current rendering/control surface and enumerate the missing per-type controls, if any, before changing UI code.
5. Record compile/link failure semantics so uniform state is committed only on successful shader updates.

## Phase 1: Design and implementation

### State and recompile flow

- Extend `UniformState` with an explicit definition refresh/merge operation that accepts newly discovered definitions and preserves `currentValue` only when both name and `kind` match.
- Keep incompatible type changes on the new default value and remove definitions absent from the successful discovery result.
- Move the successful refresh point into `WorkspaceController::updateShaders` so every compile trigger uses the same behavior.
- Ensure `updateShaders` leaves `UniformState` untouched if `PreviewRenderer::updatePreview` reports a compile/link error.
- Keep diagnostics aligned with the successful refresh count and existing error reporting.

### Uniforms panel and rendering

- Update `UniformsPanel` to render the current definitions and provide controls for scalar, vector, boolean, integer, and matrix variants using existing project conventions.
- Route every control edit through `WorkspaceController::applyUniform`, preserving the existing immediate preview update behavior.
- Ensure newly discovered definitions are usable by the panel in the same frame/state update as the successful recompile.

### Tests and validation

- Add unit coverage for `UniformState` merge behavior: additions, removals, renames, same-name/same-kind retention, same-name/type-change reset, and empty discovery.
- Add controller-level coverage where feasible for successful refresh and failed recompile retention.
- Run existing Debug and Release builds and CTest suites.
- Manually validate the edit/recompile/panel-edit workflow, including rapid recompiles and invalid shader recovery.

## Project Structure

### Documentation (this feature)

```text
specs/archive/002-refresh-uniforms-panel/
|-- spec.md
|-- plan.md
|-- research.md              # Phase 0 decisions
|-- data-model.md            # Phase 1 state model
|-- quickstart.md            # Phase 1 validation flow
`-- tasks.md                 # Phase 2 implementation tasks
```

### Source Code (repository root)

```text
src/app/workspace/WorkspaceController.cpp
src/app/workspace/WorkspaceController.h
src/app/workspace/UniformState.cpp
src/app/workspace/UniformState.h
src/rendering/shaders/UniformDefinition.h
src/rendering/shaders/UniformIntrospectionService.cpp
src/ui/panels/UniformsPanel.cpp
src/ui/panels/UniformsPanel.h
tests/unit/test_uniform_introspection.cpp
tests/unit/test_workspace_models.cpp
tests/unit/test_render_session.cpp
```

**Structure Decision**: Keep the feature inside the existing workspace/state/render/UI layers. `UniformState` owns merge and value-preservation rules, `WorkspaceController` owns transactional recompile ordering, and `UniformsPanel` remains a presentation/input adapter. No new executable, dependency, or persistence layer is needed.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| None | N/A | N/A |