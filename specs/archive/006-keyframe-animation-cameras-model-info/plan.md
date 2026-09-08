# Implementation Plan: Keyframe Animation, Phoenix `view`/`projection` Uniforms, Scene Cameras, and Model Info Panel

**Branch**: `006-keyframe-animation-cameras-model-info` | **Date**: 2026-09-08 | **Spec**: [spec.md](./spec.md)  
**Input**: Feature specification from `/specs/006-keyframe-animation-cameras-model-info/spec.md`

**Phoenix Reference Commit**: `Spontz/Phoenix` @
[`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`](https://github.com/Spontz/Phoenix/commit/75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d)
(tag `v4.2.4`, 2026-08-27). Relevant references:

- `Engine/src/core/renderer/Model.cpp`: `shader->setValue("projection", m_matProjection)`,
  `shader->setValue("view", m_matView)`, `shader->setValue("model", mesh->m_matModel)`;
  `useCamera` / `setCamera(c)` / `getSelectedCamera()` / `setAnimation(a)` /
  `PreCalc(animationTime)`; statistics `m_statNumMeshes`, `m_statNumVertices`,
  `m_statNumFaces`, `m_statNumAnimations`, `m_statNumBones`, `m_statNumCameras`.
- `Engine/src/sections/drawScene.cpp`: `m_fCameraNumber` with `-1` meaning
  "do not use a model camera" (free camera), `m_fAnimationTime` driving
  `PreCalc`, and the model camera being resolved *before* the view/projection
  matrices are read from the camera manager.

## Summary

Four coordinated additions to the existing C++20/OpenGL desktop app:

1. Promote the existing bone-only animation sampling into a general keyframe
   animation evaluator that produces node transforms as well as bone matrices,
   driven by the existing playback clock, with an explicit loop/hold policy.
2. Add `view` and `projection` as Phoenix engine-managed `mat4` uniforms,
   uploaded per frame next to the existing `MVP` / `model` / `uCameraPos`, and
   excluded from the Uniforms panel exactly as those three already are.
3. Import model cameras and add a Render-panel camera selector ("Free camera"
   plus each scene camera), where the selection drives `view`, `projection`,
   `MVP`, and `uCameraPos`, and animated cameras follow their keyframes.
4. Add a read-only "Model info" panel, toggled from the View menu, refreshed on
   every model load.

The implementation reuses the existing `WorkspaceController` render-state flow,
`PreviewRenderer` offscreen path, `UniformProvenance::PhoenixAuto` model,
`SkeletalAnimator` sampling code, and the `DockspaceHost::registerPanel` +
`show*_` + View-menu panel pattern.

## Technical Context

**Language/Version**: C++20  
**Primary Dependencies**: OpenGL 4.6 core, GLFW, Dear ImGui docking, GLM, glad,
Assimp, stb_image, `imgui_color_text_edit`  
**Storage**: No new persistent storage; camera/animation selection is session
state. `shader_editor_settings.ini` is untouched unless a selection needs to be
remembered (explicitly out of scope for this feature)  
**Testing**: Existing CTest target `shader_editor_tests`, extending
`tests/unit/test_skeletal_animator.cpp`, `test_uniform_introspection.cpp`,
`test_assimp_model_loader.cpp`, `test_preview_camera.cpp`,
`test_render_session.cpp`, `test_workspace_models.cpp`, plus a new
`test_model_info.cpp`; live ImGui/OpenGL behavior verified manually  
**Target Platform**: Windows desktop, matching the current GLFW/OpenGL and
Win32-dialog application path  
**Project Type**: Single desktop application (`src/`, `tests/`)  
**Performance Goals**: Animation and camera evaluation must stay within the
existing per-frame budget and must not regress the FPS readout added in 005;
"Model info" must be computed once per load, not per frame  
**Constraints**: No GPU resource reallocation or shader recompilation when
switching clips or cameras; `view`/`projection` must never appear in the
Uniforms panel; a scene camera must not be mutated by orbit/pan input, and the
free-camera state must survive a round trip through a scene camera  
**Scale/Scope**: One active model, one active animation clip, one active camera,
one render target, one window

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Platform compatibility**: Pass. Windows desktop GLFW/OpenGL path only; all
  new data comes from the already-linked Assimp importer and stays behind the
  existing `ModelDocument` boundary (Assimp types remain confined to
  `AssimpModelLoader.cpp`).
- **Verification**: Pass. Pure sampling, camera-resolution, and model-summary
  logic is extracted so it is unit-testable without a GL context; ImGui panel
  and live uniform upload are covered by the manual checklist.
- **UI consistency**: Pass. "Model info" reuses `registerPanel` + `show*_` bool +
  `ImGui::MenuItem` in the View menu, matching Shader Help/Config. Camera and
  animation selectors are combos placed alongside the existing Render-panel
  controls.
- **OpenGL/runtime impact**: Pass with caution. `view`/`projection` are uploaded
  in the same `applyUniforms` pass as `MVP`, so a mismatch between them and `MVP`
  is the main correctness risk; the design makes `MVP` a derived product of
  `projection * view * model` to eliminate divergence by construction.

No constitution violations are expected.

## Project Structure

### Documentation (this feature)

```text
specs/006-keyframe-animation-cameras-model-info/
├── spec.md
├── plan.md
├── research.md          # Optional if implementation discovers additional decisions
├── data-model.md        # Optional if camera/animation/model-info types expand
├── quickstart.md        # Optional manual validation checklist
└── tasks.md             # Created by the tasks step, not by this plan
```

### Source Code (repository root)

```text
src/
├── app/
│   ├── application/
│   │   ├── Application.h        # showModelInfo_ flag, drawModelInfoWindow()
│   │   └── Application.cpp      # View menu entry, panel registration, camera/animation combos, help text
│   └── workspace/
│       ├── PreviewInteractionState.h   # unchanged; remains the free-camera state
│       ├── WorkspaceController.h
│       └── WorkspaceController.cpp     # camera selection, model info refresh on load
├── rendering/
│   ├── models/
│   │   ├── ModelDocument.h      # ModelCamera, node-animation access, ModelInfoSummary source data
│   │   ├── AssimpModelLoader.cpp # import aiCamera list + per-node animation channels + stats
│   │   ├── SkeletalAnimator.h/.cpp # node-transform sampling shared by bones and cameras
│   │   ├── ModelCameraResolver.h/.cpp # NEW: pose of a scene camera at time t -> view matrix
│   │   └── ModelInfoSummary.h/.cpp    # NEW: pure ModelDocument -> displayable summary
│   ├── opengl/
│   │   ├── PreviewCamera.h/.cpp # split view()/projection(); MVP derived from them
│   │   ├── PreviewRenderer.h
│   │   └── PreviewRenderer.cpp  # upload view/projection, honor active camera source
│   └── shaders/
│       ├── RenderSession.h      # CameraSelection, clip/loop state, model info snapshot
│       └── UniformIntrospectionService.cpp # skip "view"/"projection" like MVP/model/uCameraPos
└── ui/
    └── panels/
        ├── RenderViewPanel.h/.cpp  # camera + animation selectors
        └── ModelInfoPanel.h/.cpp   # NEW: read-only model summary panel

tests/
├── CMakeLists.txt
└── unit/
    ├── test_skeletal_animator.cpp     # node/keyframe sampling, degenerate clips
    ├── test_preview_camera.cpp        # projection*view*model == MVP
    ├── test_uniform_introspection.cpp # view/projection excluded from panel
    ├── test_assimp_model_loader.cpp   # imported cameras + stats
    ├── test_render_session.cpp        # camera selection invariants
    └── test_model_info.cpp            # NEW: summary counts and empty state
```

**Structure Decision**: Keep everything inside the existing
app/workspace/rendering/ui layers. Two small new pure-logic units
(`ModelCameraResolver`, `ModelInfoSummary`) and one new panel
(`ModelInfoPanel`) are added specifically so the non-GL logic is unit-testable;
no new subsystem or dependency is introduced.

## Phase 0: Research and Decisions

1. **Keyframe animation semantics**
   - Decision: `ModelDocument::AnimationChannel` already stores position,
     rotation (quaternion), and scale keyframes in seconds; keep that shape and
     reuse it for both bones and non-bone nodes (including camera nodes).
   - Decision: rotation uses `glm::slerp`, position/scale use `glm::mix`,
     matching Phoenix's per-channel interpolation.
   - Decision: animation time is derived from the existing playback clock
     (`PlaybackClockState::elapsedSeconds()`), which is Phoenix's
     `m_fAnimationTime` equivalent, so `t` and the animation stay in lockstep.
   - Decision: end-of-clip policy is an explicit user-visible toggle in the
     Render panel, defaulting to loop (`fmod` over the clip duration); hold
     clamps to `durationSeconds`. Zero-duration or single-key clips clamp.

2. **`view` / `projection` semantics**
   - Decision: exact Phoenix names, both `mat4`, uploaded per frame in
     `PreviewRenderer::applyUniforms` next to `MVP`/`model`/`uCameraPos`.
   - Decision: they are *not* discovered as `UniformDefinition` entries at all.
     `UniformIntrospectionService::collectUniforms` already skips `MVP`,
     `uCameraPos`, and `model`; `view` and `projection` join that skip list, so
     FR-012 is satisfied by construction and nothing can render them editable.
   - Decision: `MVP` becomes `projection * view * model` rather than an
     independently built matrix, guaranteeing FR-011 without a tolerance check.
   - Rationale for skip-list over a `PhoenixAuto` provenance entry: the existing
     matrix uniforms already use the skip list; using it keeps a single,
     consistent rule for engine matrices and avoids adding a `mat4` editing path
     that the panel would otherwise have to suppress.

3. **Scene cameras**
   - Decision: import `aiScene::mCameras` into a new
     `ModelDocument::ModelCamera` (name, position, look-at, up, horizontal FOV,
     declared aspect ratio) plus the name of the scene node that carries it.
   - Decision: build the view matrix with `glm::lookAt(pos, pos + lookAt, up)`,
     matching Phoenix's corrected `processCameras` behavior noted in its
     `refactor-model-precalc-draw-split` design note; the naive
     `lookAt(pos, lookAt, up)` form is explicitly rejected because Phoenix
     documented it as producing a mirrored/backwards view.
   - Decision: Assimp reports the *half horizontal* FOV; convert to the full
     vertical FOV in degrees for `glm::perspective`, using the camera's declared
     aspect ratio when present and the viewport aspect otherwise.
   - Decision: `CameraSelection` mirrors Phoenix's `CameraNumber`: `-1` is the
     free camera (default), `>= 0` indexes `ModelDocument::cameras`. Any
     out-of-range index resolves to the free camera.
   - Decision: the free camera's `PreviewInteractionState` is never mutated while
     a scene camera is active, which satisfies FR-018's "restore unchanged"
     requirement without snapshot/restore bookkeeping.

4. **Animated cameras**
   - Decision: a camera is animated when the active clip has a channel whose
     `boneName` (node name) matches the camera's node name; resolution walks the
     `sceneNodes` parent chain with animated local transforms substituted, the
     same traversal `SkeletalAnimator` already performs for bones.
   - Decision: the resulting world matrix transforms the camera's authored
     position/look/up before `glm::lookAt`, so a static camera and an animated
     one share exactly one code path.

5. **`uCameraPos` under camera switching**
   - Decision: `uCameraPos` is always read back from the resolved active camera
     (free or scene, static or animated) each frame, never cached across frames,
     satisfying FR-017a/FR-017b with no invalidation logic.

6. **Model info extraction**
   - Decision: summary values come from the already-imported `ModelDocument`, not
     from a second Assimp parse, so the panel can never claim something that was
     not actually imported (spec edge case).
   - Decision: counts follow Phoenix's statistics set: meshes, vertices, faces
     (triangles), materials, textures (with type breakdown), bones, animations
     (name + duration), cameras (names), plus ShaderEditor's bounding box.
   - Decision: the summary is computed once in `WorkspaceController::openModel`
     and stored, so the panel draw is a pure read (FR-024, FR-026).

## Phase 1: Design

### Data Model

- **ModelCamera** (in `ModelDocument`)
  - `name: std::string`
  - `nodeName: std::string`
  - `position: glm::vec3`, `lookAt: glm::vec3`, `up: glm::vec3`
  - `horizontalFovRadians: float` (0 when unspecified)
  - `aspectRatio: float` (0 when unspecified)
  - Invariant: `up` is normalized and non-degenerate; a degenerate camera is
    dropped at import time rather than exported with NaN.

- **ModelDocument additions**
  - `std::vector<ModelCamera> cameras;`
  - Invariant: `cameras` order is the import order, so an index is stable for the
    lifetime of the loaded document.

- **CameraSelection** (in `RenderSession`)
  - `int activeCameraIndex {-1};` (`-1` = free camera)
  - Invariant: clamped to `-1` whenever the index is not a valid
    `cameras` index for the currently loaded model.

- **AnimationPlaybackPolicy** (in `RenderSession`)
  - `bool loopAnimation {true};`
  - Applied as `loop ? fmod(t, duration) : min(t, duration)` with `duration <= 0`
    short-circuiting to `0`.

- **ResolvedCamera** (transient, per frame)
  - `glm::mat4 view`, `glm::mat4 projection`, `glm::vec3 position`
  - Invariant: all components finite; `projection` built from a clamped positive
    aspect ratio.

- **ModelInfoSummary**
  - `name`, `sourcePath`
  - `meshCount`, `vertexCount`, `triangleCount`, `materialCount`
  - `hasTextures: bool`, `textureCount`, `texturesByType: vector<pair<string,int>>`
  - `hasSkeleton: bool`, `boneCount`
  - `animations: vector<{name, durationSeconds}>`
  - `cameras: vector<std::string>`
  - `boundsMin`, `boundsMax`, `boundsSize`
  - Invariant: default-constructed (empty) means "no model loaded" and the panel
    renders the empty state.

### Service/API Contracts

- `AssimpModelLoader`
  - Populate `ModelDocument::cameras` from `aiScene::mCameras`.
  - Ensure animation channels are retained for non-bone nodes (camera nodes), not
    only for nodes matching a bone name.

- `SkeletalAnimator`
  - Keep `boneTransforms(document, elapsedSeconds, animationIndex)`.
  - Add `nodeWorldTransform(document, nodeName, elapsedSeconds, animationIndex,
    loop)` returning the animated world matrix for an arbitrary node, sharing the
    existing parent-chain traversal.
  - Add a shared, testable `sampleChannel(channel, time)` helper.

- `ModelCameraResolver` (new)
  - `std::optional<ResolvedCamera> resolve(const ModelDocument&, int cameraIndex,
    float elapsedSeconds, int animationIndex, bool loop, float viewportAspect)`.
  - Returns `std::nullopt` for an invalid index so callers fall back to the free
    camera.

- `PreviewCamera`
  - Split the current combined matrix into `view(interactionState)` and
    `projection(aspectRatio)`; keep `modelMatrix()` and `position()`.
  - `viewProjection()` becomes `projection * view` so existing callers and `MVP`
    stay consistent.

- `PreviewRenderer`
  - Resolve the active camera once per frame (free vs. scene, static vs.
    animated) into a `ResolvedCamera`.
  - In `applyUniforms`: upload `view`, `projection`, `model`, `uCameraPos`, and
    `MVP = projection * view * model` from that single resolved camera.
  - Use the session's animation index and loop policy for both bone transforms
    and camera resolution so they cannot desynchronize.
  - Expose `activeModelCameraNames()` for the Render panel selector.

- `UniformIntrospectionService`
  - Extend the existing engine-matrix skip list from
    `{"MVP", "uCameraPos", "model"}` to also include `"view"` and `"projection"`.

- `WorkspaceController`
  - `selectCamera(int index)` / `selectedCameraIndex()`.
  - `modelCameraNames()`.
  - `setAnimationLooping(bool)` / `animationLooping()`.
  - `modelInfo()` returning the cached `ModelInfoSummary`.
  - `openModel()` recomputes the summary, resets `activeCameraIndex` to `-1` when
    the previous selection is no longer valid, and clamps
    `selectedAnimationIndex`.

- `RenderViewPanel`
  - Camera combo: "Free camera" plus each scene camera name, disabled entries
    replaced by an explanatory label when the model has no cameras.
  - Animation combo plus loop checkbox, shown only when clips exist.

- `ModelInfoPanel` (new)
  - `draw(const ModelInfoSummary&, bool* open)`; read-only, scrollable, grouped.

- `Application`
  - `showModelInfo_` flag, `registerPanel("model-info")`,
    `ImGui::MenuItem("Model info", nullptr, &showModelInfo_)` in the View menu,
    and `drawModelInfoWindow()`.
  - Shader Help text updated with `view` and `projection`.

## Phase 2: Implementation Approach

1. **Generalize animation sampling**
   - Extract `sampleChannel` and node-chain evaluation in `SkeletalAnimator`.
   - Add the loop/hold time normalization helper.
   - Extend `test_skeletal_animator.cpp`: interpolation between keys, quaternion
     path, single-key clip, zero-duration clip, unsorted/duplicated times,
     determinism for a repeated time value.

2. **Split camera matrices and add `view`/`projection`**
   - Split `PreviewCamera` into `view()` / `projection()`; derive
     `viewProjection()` and `MVP` from them.
   - Add `view`/`projection` to the introspection skip list.
   - Upload both in `applyUniforms`.
   - Extend `test_preview_camera.cpp` (`projection * view * model` equals the
     combined matrix) and `test_uniform_introspection.cpp` (neither name is
     discovered as a user uniform, for `mat4` and for a mismatched type).

3. **Import and resolve scene cameras**
   - Add `ModelCamera` + `ModelDocument::cameras`; populate in
     `AssimpModelLoader` (half-horizontal to full-vertical FOV conversion,
     `lookAt(pos, pos + dir, up)` convention).
   - Add `ModelCameraResolver` with static and animated paths.
   - Extend `test_assimp_model_loader.cpp` for imported cameras and add resolver
     tests for a static camera, an animated camera at two distinct times, and an
     out-of-range index.

4. **Wire camera selection through the render flow**
   - Add `CameraSelection` and the loop flag to `RenderSession`.
   - Resolve the active camera in `PreviewRenderer::renderFrame` before
     `applyUniforms`.
   - Add `WorkspaceController::selectCamera` with fallback/clamping on model load.
   - Extend `test_render_session.cpp` / `test_workspace_models.cpp` for
     out-of-range clamping and for free-camera state being untouched while a
     scene camera is active.

5. **Add Render-panel selectors**
   - Camera combo, animation combo, and loop checkbox next to the existing
     playback/background controls, with clear "model has no cameras/animations"
     states.

6. **Add the "Model info" panel**
   - Add `ModelInfoSummary` + `buildModelInfoSummary(const ModelDocument&)`.
   - Cache it on load in `WorkspaceController::openModel`; clear it when a load
     fails so no partial data is shown.
   - Add `ModelInfoPanel`, `showModelInfo_`, panel registration, and the View
     menu entry.
   - Add `test_model_info.cpp` for counts, texture-type breakdown, empty state,
     and refresh-on-reload.

7. **Update documentation**
   - README engine-provided uniforms section: add `view` and `projection`, and
     state explicitly that engine matrices are not shown in the Uniforms panel.
   - README panels/workflow section: camera selector, animation selector and loop
     policy, and the "Model info" panel.
   - In-app Shader Help: add `view` and `projection`.

## Verification Plan

### Automated

```powershell
cmake --build build-vcpkg --config Debug
ctest --test-dir build-vcpkg --output-on-failure -C Debug -R shader_editor_tests
```

Expected new/updated coverage:

- `test_skeletal_animator.cpp`: channel interpolation, degenerate clips, loop vs.
  hold, determinism.
- `test_preview_camera.cpp`: `projection * view * model == MVP`.
- `test_uniform_introspection.cpp`: `view`/`projection` never discovered as
  editable uniforms.
- `test_assimp_model_loader.cpp`: cameras imported with name/FOV/transform.
- `test_render_session.cpp` / `test_workspace_models.cpp`: camera-index clamping
  and fallback on model load.
- `test_model_info.cpp`: summary counts, texture types, empty state, refresh.

### Manual

1. Import an animated model, play the timeline, and confirm the geometry animates
   and holds a stable pose when paused.
2. Switch the active animation clip and confirm the change applies without a
   reload and without losing unsaved shader edits.
3. Toggle loop/hold and confirm behavior past the clip duration.
4. Load a shader using `uniform mat4 view;` and `uniform mat4 projection;` and
   confirm it renders identically to the `MVP` version.
5. Confirm `view` and `projection` do not appear in the Uniforms panel.
6. Import a model with cameras; switch between "Free camera" and each scene
   camera and confirm framing changes within one frame.
7. With a view-dependent (specular/fresnel) shader, confirm shading changes on
   camera switch, proving `uCameraPos` followed the selection.
8. Orbit/pan while a scene camera is active, switch back to "Free camera", and
   confirm the previous free-camera framing is exactly restored.
9. Select an animated camera and confirm the framing animates over a 10-second
   run in sync with the geometry.
10. Open View > "Model info" with no model loaded (empty state), then load a
    model and confirm all fields populate; load a second model and confirm the
    panel refreshes without being reopened.
11. Load a model with no cameras and no animations and confirm both selectors
    show explicit "not available" states without errors.

## Complexity Tracking

> No Constitution Check violations; this section is intentionally empty.
