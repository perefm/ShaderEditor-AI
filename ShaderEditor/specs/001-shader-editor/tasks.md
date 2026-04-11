# Tasks: Multiplatform Shader Editor

**Input**: Design documents from `/specs/001-shader-editor/`
**Prerequisites**: plan.md (required), spec.md (required for user stories), research.md, data-model.md, contracts/

**Tests**: Every story and shared foundation change includes at least one
verification task. Catch2 covers non-rendering logic, and manual smoke
validation covers render, docking, update triggers, and workspace behavior.

**Organization**: Tasks are grouped by user story to enable independent
implementation and testing of each story.

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

## Path Conventions

- **Single desktop project**: `src/`, `tests/`, `assets/`, `.vscode/` at repository root
- Paths below assume the structure defined in `plan.md`

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Initialize the CMake-based desktop application and VS Code workflow

- [X] T001 Create the CMake project skeleton in CMakeLists.txt
- [X] T002 Create the application entry point in src/app/application/main.cpp
- [X] T003 [P] Create VS Code build and debug tasks in .vscode/tasks.json
- [X] T004 [P] Create VS Code launch configuration in .vscode/launch.json
- [X] T005 [P] Add project workspace defaults in .vscode/settings.json
- [X] T006 [P] Create asset directory placeholders for built-in resources in assets/primitives/.gitkeep and assets/shaders/.gitkeep

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Build the shared application, rendering, persistence, and UI foundations required by all stories

**CRITICAL**: No user story work can begin until this phase is complete

- [X] T007 Create the application shell and lifecycle coordinator in src/app/application/Application.h and src/app/application/Application.cpp
- [X] T008 [P] Create the platform window and OpenGL context bootstrap in src/app/platform/WindowContext.h and src/app/platform/WindowContext.cpp
- [X] T009 [P] Create the render session model in src/rendering/shaders/RenderSession.h
- [X] T010 [P] Create the shader document model in src/editor/ShaderPairDocument.h
- [X] T011 [P] Create the uniform definition model in src/rendering/shaders/UniformDefinition.h
- [X] T012 [P] Create the preview primitive model in src/rendering/geometry/PreviewPrimitive.h
- [X] T013 Implement the shader file service in src/services/files/ShaderFileService.h and src/services/files/ShaderFileService.cpp
- [X] T014 Implement the shader program compiler/linker service in src/rendering/shaders/ShaderProgramService.h and src/rendering/shaders/ShaderProgramService.cpp
- [X] T015 Implement primitive registration and mesh loading in src/rendering/geometry/PrimitiveLibrary.h and src/rendering/geometry/PrimitiveLibrary.cpp
- [X] T016 Implement the dockspace host and panel registry in src/ui/dockspace/DockspaceHost.h and src/ui/dockspace/DockspaceHost.cpp
- [X] T017 Implement diagnostics and status message state in src/app/workspace/DiagnosticsState.h and src/app/workspace/DiagnosticsState.cpp
- [X] T018 [P] Add Catch2 test target wiring for core logic in tests/CMakeLists.txt
- [X] T019 [P] Add foundational unit tests for shader document and uniform models in tests/unit/test_workspace_models.cpp

**Checkpoint**: Foundation ready; user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Edit and Save Shaders (Priority: P1) MVP

**Goal**: Let users load, edit, save, and protect a paired vertex/fragment shader workflow

**Independent Test**: Load a vertex shader and fragment shader, edit both,
save them, reopen them, and confirm the saved content matches the edits.

### Verification for User Story 1

- [X] T020 [P] [US1] Add unit tests for file loading, save behavior, and dirty-state transitions in tests/unit/test_shader_file_service.cpp
- [X] T021 [P] [US1] Add manual validation steps for shader open/save workflows in specs/001-shader-editor/quickstart.md

### Implementation for User Story 1

- [X] T022 [P] [US1] Implement shader editor buffer state in src/editor/ShaderEditorState.h and src/editor/ShaderEditorState.cpp
- [X] T023 [US1] Implement open/save/discard commands in src/app/workspace/WorkspaceController.h and src/app/workspace/WorkspaceController.cpp
- [X] T024 [US1] Implement the shader editor panel UI in src/ui/panels/ShaderEditorPanel.h and src/ui/panels/ShaderEditorPanel.cpp
- [X] T025 [US1] Implement unsaved-change prompts and error presentation in src/ui/widgets/DocumentDialogs.h and src/ui/widgets/DocumentDialogs.cpp
- [X] T026 [US1] Wire editor panel actions into the application shell in src/app/application/Application.cpp

**Checkpoint**: User Story 1 should be fully functional and testable independently

---

## Phase 4: User Story 2 - Preview Shaders on Multiple Models and Uniforms (Priority: P2)

**Goal**: Let users preview the active shader pair on built-in primitives, trigger shader updates, and change uniform values live

**Independent Test**: Open a shader pair, switch between plane, cube, torus,
and at least two additional primitives, press `Ctrl+Enter`, click the update
button, edit exposed uniform values, and confirm the render view reflects each
change.

### Verification for User Story 2

- [X] T027 [P] [US2] Add unit tests for primitive selection, shader update triggers, and uniform value application in tests/unit/test_render_session.cpp
- [X] T028 [P] [US2] Add manual validation steps for preview model switching, update triggers, and uniform editing in specs/001-shader-editor/quickstart.md

### Implementation for User Story 2

- [X] T029 [P] [US2] Implement built-in primitive meshes for plane, cube, torus, sphere, and cylinder in src/rendering/geometry/BuiltInPrimitives.h and src/rendering/geometry/BuiltInPrimitives.cpp
- [X] T030 [US2] Implement the OpenGL preview renderer in src/rendering/opengl/PreviewRenderer.h and src/rendering/opengl/PreviewRenderer.cpp
- [X] T031 [US2] Implement shader reflection and uniform discovery in src/rendering/shaders/UniformIntrospectionService.h and src/rendering/shaders/UniformIntrospectionService.cpp
- [X] T032 [US2] Implement runtime uniform state management in src/app/workspace/UniformState.h and src/app/workspace/UniformState.cpp
- [X] T033 [US2] Implement shader update actions for `Ctrl+Enter` and the update button in src/ui/panels/ShaderEditorPanel.cpp and src/app/workspace/WorkspaceController.cpp
- [X] T034 [US2] Implement the render view panel in src/ui/panels/RenderViewPanel.h and src/ui/panels/RenderViewPanel.cpp
- [X] T035 [US2] Implement the uniform controls panel in src/ui/panels/UniformsPanel.h and src/ui/panels/UniformsPanel.cpp
- [X] T036 [US2] Wire render, primitive, shader update, and uniform updates through the workspace controller in src/app/workspace/WorkspaceController.cpp
- [X] T037 [US2] Surface compile, link, and render failures in the diagnostics state and UI via src/app/workspace/DiagnosticsState.cpp and src/ui/panels/RenderViewPanel.cpp

**Checkpoint**: User Stories 1 and 2 should both work independently

---

## Phase 5: User Story 3 - Organize a Dockable Workspace (Priority: P3)

**Goal**: Let users arrange the editor, render, uniforms, diagnostics, and shader errors panels in a dockable workspace

**Independent Test**: Dock and undock the editor, render, uniforms,
diagnostics, and shader errors panels, then continue editing shaders and
previewing them without losing workspace state.

### Verification for User Story 3

- [X] T038 [P] [US3] Add manual validation steps for dock and layout persistence workflows in specs/001-shader-editor/quickstart.md
- [X] T039 [P] [US3] Add unit tests for workspace layout serialization in tests/unit/test_workspace_layout.cpp

### Implementation for User Story 3

- [X] T040 [US3] Implement the workspace layout state model in src/app/workspace/WorkspaceLayoutState.h and src/app/workspace/WorkspaceLayoutState.cpp
- [X] T041 [US3] Implement the diagnostics panel UI in src/ui/panels/DiagnosticsPanel.h and src/ui/panels/DiagnosticsPanel.cpp
- [X] T042 [US3] Implement the dedicated shader errors panel UI in src/ui/panels/ShaderErrorsPanel.h and src/ui/panels/ShaderErrorsPanel.cpp
- [X] T043 [US3] Implement layout save and restore services in src/services/persistence/LayoutPersistenceService.h and src/services/persistence/LayoutPersistenceService.cpp
- [X] T044 [US3] Wire dockable panel registration and layout restore into the application shell in src/app/application/Application.cpp

**Checkpoint**: All user stories should now be independently functional

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Finish documentation, validation, and cross-story quality work

- [X] T045 [P] Add cross-platform build notes and dependency setup guidance in specs/001-shader-editor/quickstart.md
- [X] T046 Improve application-level logging and user-visible status reporting in src/app/application/Application.cpp and src/app/workspace/DiagnosticsState.cpp
- [X] T047 [P] Add sample shader assets for smoke testing in assets/shaders/basic.vert and assets/shaders/basic.frag
- [X] T048 [P] Add final smoke-test checklist coverage for quickstart validation in specs/001-shader-editor/quickstart.md
- [ ] T049 Run the documented build and smoke validation flow and capture any follow-up fixes in specs/001-shader-editor/quickstart.md
- [X] T074 [P] Create a repository README with application overview, feature usage, and build instructions in README.md
- [X] T075 Build and validate the Release configuration of the application in build-vcpkg/ and document the exact command flow in specs/001-shader-editor/quickstart.md

---

## Phase 7: Foundational Enhancement - GLM Math Refactor

**Purpose**: Establish a single GLM-based math layer that all new preview navigation and uniform features build on

**CRITICAL**: Complete this phase before implementing mouse-driven camera control or matrix uniform editing

- [X] T050 Update dependency and build wiring so GLM is a required library in vcpkg.json, CMakeLists.txt, and specs/001-shader-editor/quickstart.md
- [X] T051 [P] Add GLM-backed preview camera and interaction state models in src/rendering/opengl/PreviewCamera.h, src/rendering/opengl/PreviewCamera.cpp, src/app/workspace/PreviewInteractionState.h, and src/app/workspace/PreviewInteractionState.cpp
- [X] T052 [P] Refactor render-domain math types to use GLM vectors and matrices in src/rendering/geometry/PreviewPrimitive.h, src/rendering/shaders/UniformDefinition.h, src/rendering/shaders/RenderSession.h, and src/app/workspace/UniformState.h
- [X] T053 Refactor the OpenGL preview pipeline to use GLM for transforms, camera matrices, and uniform upload helpers in src/rendering/opengl/PreviewRenderer.h and src/rendering/opengl/PreviewRenderer.cpp
- [X] T054 [P] Add unit coverage for GLM-backed camera/math helpers and render-session value mapping in tests/unit/test_preview_camera.cpp and tests/unit/test_render_session.cpp

**Checkpoint**: GLM is the only math layer used by the preview pipeline and shared render-state models

---

## Phase 8: User Story 4 - Rotate and Move the 3D Scene with the Mouse (Priority: P1)

**Goal**: Let users rotate the preview scene with left-drag and move the scene with right-drag directly in the render panel

**Independent Test**: Open a shader pair, press and drag the left mouse button in the render panel to rotate the previewed primitive, then press and drag the right mouse button to pan the scene, and confirm the preview updates continuously without breaking docking or editing.

### Verification for User Story 4

- [X] T055 [P] [US4] Add manual validation steps for left-drag rotation and right-drag panning in specs/001-shader-editor/quickstart.md
- [X] T056 [P] [US4] Add unit tests for preview interaction deltas, camera orbit limits, and pan accumulation in tests/unit/test_preview_camera.cpp

### Implementation for User Story 4

- [X] T057 [US4] Implement mouse capture, drag-state tracking, and viewport hit-testing in src/ui/panels/RenderViewPanel.h, src/ui/panels/RenderViewPanel.cpp, and src/app/application/Application.cpp
- [X] T058 [US4] Implement orbit and pan commands in the workspace layer in src/app/workspace/WorkspaceController.h, src/app/workspace/WorkspaceController.cpp, src/app/workspace/PreviewInteractionState.h, and src/app/workspace/PreviewInteractionState.cpp
- [X] T059 [US4] Apply GLM-based orbit and pan camera transforms to the render preview in src/rendering/opengl/PreviewCamera.h, src/rendering/opengl/PreviewCamera.cpp, and src/rendering/opengl/PreviewRenderer.cpp
- [X] T060 [US4] Surface interaction status and reset behavior in the render panel UI in src/ui/panels/RenderViewPanel.cpp and src/app/application/Application.cpp

**Checkpoint**: User Story 4 should be fully functional and testable independently

---

## Phase 9: User Story 5 - Edit Extended Uniform Types Including Matrices (Priority: P2)

**Goal**: Let users inspect and edit vec2, vec3, vec4, mat2, mat3, and mat4 uniforms from the workspace and apply them live to the preview

**Independent Test**: Open shaders that expose vec2, vec3, vec4, mat2, mat3, and mat4 uniforms, edit each value shape from the uniform panel, trigger an update, and confirm the preview or diagnostics reflect the applied values without crashing.

### Verification for User Story 5

- [X] T061 [P] [US5] Add unit tests for uniform discovery, default values, and shape-safe updates for vec2/vec3/vec4/mat2/mat3/mat4 in tests/unit/test_uniform_introspection.cpp and tests/unit/test_workspace_models.cpp
- [X] T062 [P] [US5] Add manual validation steps for extended uniform editing in specs/001-shader-editor/quickstart.md

### Implementation for User Story 5

- [X] T063 [US5] Extend uniform metadata and value storage for vector and matrix types in src/rendering/shaders/UniformDefinition.h, src/app/workspace/UniformState.h, and src/app/workspace/UniformState.cpp
- [X] T064 [US5] Extend shader uniform introspection to classify vec2, vec3, vec4, mat2, mat3, and mat4 accurately in src/rendering/shaders/UniformIntrospectionService.h and src/rendering/shaders/UniformIntrospectionService.cpp
- [X] T065 [US5] Implement matrix and vector editing controls in src/ui/panels/UniformsPanel.h, src/ui/panels/UniformsPanel.cpp, and src/app/application/Application.cpp
- [X] T066 [US5] Implement GLM-backed uniform upload paths for extended types in src/rendering/opengl/PreviewRenderer.cpp and src/rendering/shaders/RenderSession.h
- [X] T067 [US5] Improve diagnostics for unsupported or invalid uniform edits in src/app/workspace/DiagnosticsState.cpp, src/app/workspace/WorkspaceController.cpp, and src/ui/panels/ShaderErrorsPanel.cpp

**Checkpoint**: User Stories 4 and 5 should both work independently

---

## Phase 10: User Story 6 - Make the Codebase Consistently GLM-Based and Well Commented (Priority: P3)

**Goal**: Keep the rendering and workspace code maintainable by completing the GLM refactor across remaining math code and adding clear explanatory comments throughout the codebase

**Independent Test**: Review the source tree and confirm all remaining custom math helpers have been replaced by GLM-based equivalents, the project still builds and runs, and each core subsystem contains comments that explain non-obvious control flow, OpenGL state usage, and UI interaction handling.

### Verification for User Story 6

- [X] T068 [P] [US6] Add a manual code-review checklist for GLM-only math usage and comment coverage in specs/001-shader-editor/quickstart.md
- [ ] T069 [P] [US6] Run full regression verification for build, tests, preview navigation, docking, and extended uniforms in specs/001-shader-editor/quickstart.md

### Implementation for User Story 6

- [X] T070 [US6] Remove remaining handwritten math helpers and align all render math usage to GLM in src/rendering/opengl/PreviewRenderer.cpp, src/rendering/opengl/PreviewCamera.cpp, and src/rendering/geometry/BuiltInPrimitives.cpp
- [X] T071 [P] [US6] Add explanatory comments to the application and workspace flow in src/app/application/Application.cpp, src/app/workspace/WorkspaceController.cpp, and src/app/platform/WindowContext.cpp
- [X] T072 [P] [US6] Add explanatory comments to render and shader subsystems in src/rendering/opengl/PreviewRenderer.cpp, src/rendering/opengl/PreviewCamera.cpp, src/rendering/shaders/ShaderProgramService.cpp, and src/rendering/shaders/UniformIntrospectionService.cpp
- [X] T073 [P] [US6] Add explanatory comments to UI panel and supporting widget code in src/ui/panels/RenderViewPanel.cpp, src/ui/panels/UniformsPanel.cpp, src/ui/panels/ShaderEditorPanel.cpp, src/ui/panels/DiagnosticsPanel.cpp, src/ui/panels/ShaderErrorsPanel.cpp, and src/ui/widgets/DocumentDialogs.cpp

**Checkpoint**: All enhancement stories should now be independently functional and the codebase should be maintainable for further preview work

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies; can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion; blocks all user stories
- **User Story 1 (Phase 3)**: Depends on Foundational completion; MVP entry point
- **User Story 2 (Phase 4)**: Depends on Foundational completion and integrates with the editor workflow from US1
- **User Story 3 (Phase 5)**: Depends on Foundational completion and benefits from US1/US2 panels being available
- **Polish (Phase 6)**: Depends on all desired original user stories being complete
- **Foundational Enhancement (Phase 7)**: Depends on Phases 3-6 being stable enough to refactor safely; blocks enhancement stories
- **User Story 4 (Phase 8)**: Depends on Phase 7 completion
- **User Story 5 (Phase 9)**: Depends on Phase 7 completion and integrates with the preview flow from US4
- **User Story 6 (Phase 10)**: Depends on Phases 7-9 completion

### User Story Dependencies

- **User Story 1 (P1)**: Can start after Foundational; no dependency on other stories
- **User Story 2 (P2)**: Depends on the active shader document/editor pipeline from US1
- **User Story 3 (P3)**: Depends on the editor, render, uniform, diagnostics, and shader errors panels delivered by earlier stories
- **User Story 4 (P1 enhancement)**: Depends on the render panel and OpenGL preview pipeline plus the GLM foundation from Phase 7
- **User Story 5 (P2 enhancement)**: Depends on the GLM foundation from Phase 7 and benefits from the live preview path exercised by US4
- **User Story 6 (P3 enhancement)**: Depends on all enhancement stories so comments and final GLM cleanup reflect the settled architecture

### Within Each User Story

- Verification tasks should be written before or alongside implementation
- State and model definitions before controller logic
- Controller logic before panel wiring
- Story must pass its independent validation before moving on

### Parallel Opportunities

- Setup tasks `T003`-`T006` can run in parallel after `T001`
- Foundational model tasks `T009`-`T012` and test bootstrap tasks `T018`-`T019` can run in parallel
- User Story 1 tasks `T020`-`T022` can run in parallel before controller/UI integration
- User Story 2 tasks `T027`-`T029` can run in parallel before renderer/controller integration
- User Story 3 tasks `T038`-`T039` can run in parallel before layout integration
- Polish tasks `T045`, `T047`, and `T048` can run in parallel
- Polish tasks `T074` and `T075` can run in parallel with other final validation/documentation work once implementation is stable
- Phase 7 tasks `T051`, `T052`, and `T054` can run in parallel after `T050`
- User Story 4 verification tasks `T055`-`T056` can run in parallel before `T057`-`T060`
- User Story 5 verification tasks `T061`-`T062` can run in parallel before `T063`-`T067`
- User Story 6 comment-focused tasks `T071`-`T073` can run in parallel after `T068`-`T070`

---

## Parallel Example: User Story 4

```bash
# Launch independent verification work for User Story 4 together:
Task: "Add manual validation steps for left-drag rotation and right-drag panning in specs/001-shader-editor/quickstart.md"
Task: "Add unit tests for preview interaction deltas, camera orbit limits, and pan accumulation in tests/unit/test_preview_camera.cpp"

# Launch implementation work that can proceed in parallel after verification:
Task: "Implement mouse capture, drag-state tracking, and viewport hit-testing in src/ui/panels/RenderViewPanel.cpp and src/app/application/Application.cpp"
Task: "Implement orbit and pan commands in src/app/workspace/WorkspaceController.cpp and src/app/workspace/PreviewInteractionState.cpp"
```

---

## Implementation Strategy

### MVP First (User Story 1 Only)

1. Complete Phase 1: Setup
2. Complete Phase 2: Foundational
3. Complete Phase 3: User Story 1
4. Validate shader loading, editing, saving, and unsaved-change handling
5. Stop for review if a minimal editor-only milestone is desired

### Incremental Delivery

1. Deliver the editable shader workspace first
2. Add live preview, shader update triggers, primitives, and uniform editing next
3. Add workspace docking, the dedicated shader errors panel, and layout persistence after core workflows are stable
4. Add the GLM math foundation and mouse-driven scene navigation
5. Add extended vector and matrix uniform editing
6. Finish with comment coverage, GLM cleanup, sample assets, documentation, and smoke validation

### Suggested MVP Scope

- Setup and Foundational phases
- User Story 1 only for the original feature
- For the enhancement backlog, Phase 7 plus User Story 4 only

---

## Notes

- All tasks follow the required checklist format with IDs and file paths
- User Story tasks include `[US1]` through `[US6]` labels
- Exact implementation files can expand beneath the planned directories if needed
- Validation is split between Catch2 unit coverage and documented manual render checks
- Enhancement tasks derive from the current spec plus the newly requested scope for mouse navigation, GLM-only math, broader uniform support, and explanatory comments
