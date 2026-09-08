# Feature Specification: Keyframe Animation, Phoenix `view`/`projection` Uniforms, Scene Cameras, and Model Info Panel

**Feature Branch**: `006-keyframe-animation-cameras-model-info`  
**Created**: 2026-09-08  
**Status**: Implemented  
**Input**: User description: "haz una nueva spec que soporte animaciones por keyframes (como en Phoenix) tambien ha de darse soporte a las uniforms como \"view\" y \"projection\" del mismo modo que lo hace Phoenix, por lo que esas uniforms no se han de mostrar en el panel de uniforms y ha de dejarse documentado. En el render se ha de poder elegir si se usa una cámara libre o una de las cámaras de la escena (si las tiene). En el caso de que la cámara esté animada por keyframes, se ha de soportar tambien. Finalmente, quiero que añadas un panel llamada \"Model info\" que se acceda desde el menu \"View\" y muestre la informacion del modelo cargado: nombre, numero de vértices, triángulos, mallas, numero de materiales, si tiene texturas o no, etc... todo lo que puedas sacar de assimp. Esta informacion se ha de refrescar cada vez que se carga un modelo nuevo."

**Phoenix Reference Commit**: Behavior in this spec is derived from the
`Spontz/Phoenix` GitHub repository at commit
[`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`](https://github.com/Spontz/Phoenix/commit/75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d)
(tag `v4.2.4`, 2026-08-27), specifically:

- `Engine/src/core/renderer/Model.cpp`: uploads `projection` and `view` as
  per-draw engine matrices (`shader->setValue("projection", m_matProjection)`,
  `shader->setValue("view", m_matView)`) alongside the per-mesh `model` matrix;
  exposes `useCamera`, `setCamera(index)`, `getSelectedCamera()`,
  `setAnimation(index)`, `PreCalc(animationTime)`, and the statistics
  `m_statNumMeshes`, `m_statNumVertices`, `m_statNumFaces`,
  `m_statNumAnimations`, `m_statNumBones`, `m_statNumCameras`.
- `Engine/src/sections/drawScene.cpp`: selects a scene camera by index with
  `-1` meaning "do not use a model camera" (free camera), drives animation with
  an `AnimationTime` value, and resolves the model camera before the view and
  projection matrices are uploaded.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Play a model's keyframe animation like Phoenix (Priority: P1)

As a shader author previewing Phoenix content, I want the imported model's
keyframe animation clips to play in the Render preview with the same time model
Phoenix uses, so animated meshes, nodes, and skeletons look the same in
ShaderEditor as they do in the demo engine.

**Why this priority**: Animation playback is the foundation for both animated
skinning and animated cameras; the rest of this feature depends on it.

**Independent Test**: Import a model containing at least one animation clip,
select the clip in the Render panel, play the timeline, and verify the geometry
deforms/moves over time and stops or loops predictably when the clip ends.

**Acceptance Scenarios**:

1. **Given** an imported model exposes one or more animation clips, **When** the
   Render panel is shown, **Then** the user can see the available clips and
   select which one is active, identified by clip name and index.
2. **Given** an animation clip is selected and playback is running, **When**
   playback time advances, **Then** node and bone transforms are sampled from
   the clip's position, rotation, and scale keyframes and interpolated between
   surrounding keys.
3. **Given** rotation keyframes are present, **When** the animation is sampled
   between two keys, **Then** rotations are interpolated without gimbal
   artifacts or visible popping at key boundaries.
4. **Given** playback time exceeds the clip duration, **When** the animation
   continues, **Then** the clip loops or holds according to a user-visible,
   explicitly documented option, and never produces invalid transforms.
5. **Given** the user scrubs or resets playback time, **When** the next frame
   renders, **Then** the animated pose corresponds to the new time
   deterministically (the same time always yields the same pose).
6. **Given** a model has no animation clips, **When** it is rendered, **Then**
   the animation controls indicate that no clips are available and the model
   renders in its bind/static pose without errors.
7. **Given** an animated model, **When** the user switches the active animation
   clip, **Then** the change applies without reloading the model or the shaders.

---

### User Story 2 - Receive Phoenix `view` and `projection` uniforms automatically (Priority: P1)

As a shader author, I want ShaderEditor to supply `view` and `projection` as
engine-managed matrices exactly as Phoenix does, so Phoenix shaders that build
their own transforms from `projection * view * model` render correctly without
me creating manual uniform entries.

**Why this priority**: Without these matrices, most Phoenix scene shaders cannot
be previewed correctly at all.

**Independent Test**: Load a Phoenix shader that declares `uniform mat4 view;`
and `uniform mat4 projection;` and computes `gl_Position = projection * view *
model * vec4(aPos, 1.0);`, render it, orbit the preview camera, and verify the
geometry transforms correctly while neither uniform appears as editable in the
Uniforms panel.

**Acceptance Scenarios**:

1. **Given** a shader declares `uniform mat4 view;`, **When** it renders,
   **Then** the uploaded value is the current preview camera's view matrix for
   that frame.
2. **Given** a shader declares `uniform mat4 projection;`, **When** it renders,
   **Then** the uploaded value is the current preview projection matrix, built
   from the effective Render viewport aspect ratio.
3. **Given** a shader declares `view`, `projection`, and `model`, **When** it
   renders, **Then** `projection * view * model` produces the same clip-space
   transform as the existing engine-provided `MVP` uniform for the same frame.
4. **Given** `view` or `projection` is declared, **When** the Uniforms panel is
   shown, **Then** neither name is listed as a user-editable uniform, matching
   the existing treatment of `MVP`, `model`, and `uCameraPos`.
5. **Given** the camera is moved, the Render panel is resized, or the active
   camera source changes, **When** the next frame renders, **Then** `view` and
   `projection` reflect the new state without shader recompilation.
6. **Given** a shader omits `view` and/or `projection`, **When** it renders,
   **Then** no warning or error is produced for the omitted names.
7. **Given** a shader declares `view` or `projection` with a non-`mat4` type,
   **When** it renders, **Then** the automatic value is not uploaded and the
   mismatch is surfaced consistently with existing uniform diagnostics.

---

### User Story 3 - Choose between the free camera and a scene camera (Priority: P1)

As a shader author, I want the Render panel to let me choose between the free
(orbit/pan) preview camera and any camera embedded in the loaded model, so I can
preview the scene exactly through the authored camera as Phoenix's
`CameraNumber` does.

**Why this priority**: Framing through the authored camera is required to
validate Phoenix scenes, and it directly determines the `view`/`projection`
values from User Story 2.

**Independent Test**: Import a model containing at least one camera, switch the
Render panel camera selector from "Free camera" to a scene camera, and verify the
preview framing changes to the authored camera while manual orbit input no longer
overrides that framing.

**Acceptance Scenarios**:

1. **Given** the Render panel is visible, **When** the user opens the camera
   selector, **Then** "Free camera" is available and is the default selection.
2. **Given** the loaded model contains cameras, **When** the user opens the
   camera selector, **Then** each scene camera is listed and identified by name
   and index.
3. **Given** the loaded model contains no cameras, **When** the user opens the
   camera selector, **Then** only "Free camera" is selectable and the UI states
   that the model has no cameras.
4. **Given** a scene camera is selected, **When** a frame renders, **Then**
   `view`, `projection`, `MVP`, and `uCameraPos` are all derived from that
   camera's position, orientation, and field of view, using the effective Render
   viewport aspect ratio.
5. **Given** a scene camera is selected, **When** the user returns to
   "Free camera", **Then** the previous free-camera orbit/pan/zoom state is
   restored unchanged.
6. **Given** the user switches between any two camera sources, **When** the next
   frame renders, **Then** `uCameraPos` equals the world-space position of the
   newly selected camera, so view-dependent shading (specular, fresnel, view
   vectors) updates immediately.
7. **Given** a new model is loaded while a scene camera was selected, **When**
   loading completes, **Then** the selection falls back to "Free camera" unless
   the new model provides an equivalent camera, and the selector never points at
   a non-existent camera index.

---

### User Story 4 - Support keyframe-animated scene cameras (Priority: P2)

As a shader author, I want a selected scene camera that is animated by keyframes
to move over playback time, so camera moves authored in the model file are
previewed exactly as Phoenix plays them.

**Why this priority**: Animated cameras are a common Phoenix authoring pattern,
but they build on static scene-camera selection and animation playback.

**Independent Test**: Import a model with an animated camera, select that camera,
play the timeline, and verify the preview framing changes over time consistently
with the authored camera motion.

**Acceptance Scenarios**:

1. **Given** a selected scene camera is targeted by animation channels in the
   active clip, **When** playback time advances, **Then** the camera's world
   transform is sampled from those keyframes and the preview framing animates.
2. **Given** an animated camera is active, **When** each frame renders, **Then**
   `view`, `projection`, `MVP`, and `uCameraPos` correspond to the camera pose at
   the current playback time.
3. **Given** playback is paused, **When** frames continue rendering, **Then** the
   animated camera holds the pose at the paused time.
4. **Given** the selected camera is not animated in the active clip, **When**
   playback runs, **Then** the camera remains at its authored static pose and the
   rest of the animated model still animates.
5. **Given** the active animation clip is changed, **When** the next frame
   renders, **Then** the animated camera follows the newly selected clip's
   channels.

---

### User Story 5 - Inspect the loaded model in a "Model info" panel (Priority: P2)

As a user, I want a "Model info" panel opened from the View menu that summarizes
everything ShaderEditor can extract from the loaded model, so I can verify what
was actually imported before debugging a shader.

**Why this priority**: It is a diagnostic aid that makes the rest of the feature
verifiable, but it does not block rendering.

**Independent Test**: Open View > "Model info" with no model loaded, verify the
empty state, then import a model and verify the panel immediately shows the new
model's name, counts, materials, textures, animations, and cameras.

**Acceptance Scenarios**:

1. **Given** the application is running, **When** the user opens the View menu,
   **Then** a "Model info" entry toggles the panel using the existing panel
   visibility pattern.
2. **Given** no model is loaded, **When** the panel is shown, **Then** it
   displays a clear empty state instead of stale or zeroed data.
3. **Given** a model is loaded, **When** the panel is shown, **Then** it displays
   at least: model name, source file path, total meshes, total vertices, total
   triangles/faces, total materials, whether textures are present, total
   textures with their types, whether the model has a skeleton, bone count,
   animation count with clip names and durations, camera count with camera
   names, and the model's bounding box/size.
4. **Given** a model is loaded, **When** the user loads a different model,
   **Then** the panel refreshes to the new model's data without requiring the
   panel to be closed and reopened.
5. **Given** a model fails to load, **When** the panel is shown, **Then** it does
   not present partial or misleading statistics and defers the failure reason to
   the existing diagnostics flow.
6. **Given** a model has many meshes, materials, or clips, **When** the panel is
   shown, **Then** the content stays readable via scrolling or grouping and does
   not stall the UI.

### Edge Cases

- What happens when an animation clip has zero duration or a single keyframe?
  Sampling MUST clamp to that key and MUST NOT divide by zero.
- What happens when keyframe times are unsorted, duplicated, or contain gaps for
  one channel but not others? Each channel MUST be sampled independently and
  degenerate input MUST NOT produce NaN transforms or crashes.
- What happens when a selected camera index becomes invalid after loading a new
  model? The selection MUST fall back to "Free camera".
- What happens when a scene camera has no usable field of view or aspect ratio
  data? The system MUST fall back to the preview's existing projection
  parameters and document the fallback.
- What happens when the Render viewport is near-zero while a scene camera is
  active? The projection MUST stay numerically valid, as already required for
  `aspectRatio`.
- What happens when a shader declares `view`/`projection` but no model is loaded
  (built-in primitive preview)? The matrices MUST still be supplied from the
  active preview camera.
- What happens when the model file reports statistics ShaderEditor does not
  import (for example unsupported light or texture types)? The "Model info"
  panel MUST only report what was actually imported, without inventing values.
- Could this feature break OpenGL context setup, shader/resource loading, or UI
  responsiveness? Animation sampling and camera resolution MUST run per frame
  without reallocating GPU resources or reloading shaders.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The system MUST play keyframe animation clips imported from the
  model file, sampling position, rotation, and scale channels over playback time.
- **FR-002**: Rotation channels MUST be interpolated using quaternion
  interpolation, and position/scale channels MUST be linearly interpolated
  between surrounding keys.
- **FR-003**: Animation sampling MUST be deterministic: the same playback time
  MUST always produce the same pose.
- **FR-004**: The Render panel MUST let the user select the active animation clip
  by name/index when the model exposes more than one clip.
- **FR-005**: The Render panel MUST expose a documented end-of-clip behavior
  (loop or hold) and MUST NOT produce invalid transforms past the clip duration.
- **FR-006**: Animation playback MUST be driven by the existing playback clock so
  that it stays synchronized with the `t` uniform.
- **FR-007**: Changing the active animation clip MUST NOT reload the model,
  shaders, textures, or uniform values.
- **FR-008**: The system MUST recognize `view` and `projection` as
  Phoenix-compatible engine-managed uniforms when declared as `mat4`.
- **FR-009**: `view` MUST equal the active preview camera's view matrix for the
  current frame.
- **FR-010**: `projection` MUST equal the active preview projection matrix for
  the current frame, using the effective Render viewport aspect ratio.
- **FR-011**: `projection * view * model` MUST match the value uploaded to the
  existing `MVP` uniform for the same frame.
- **FR-012**: `view` and `projection` MUST NOT be shown or editable in the
  Uniforms panel, consistent with the existing handling of `MVP`, `model`, and
  `uCameraPos`.
- **FR-013**: Shaders that omit `view` and/or `projection` MUST compile and
  render without warnings related to their absence.
- **FR-014**: Declaring `view` or `projection` with an incompatible type MUST NOT
  upload the automatic value and MUST surface the mismatch consistently with
  existing uniform diagnostics.
- **FR-015**: The system MUST import cameras defined in the model file, including
  their name, position, look direction, up vector, and field of view where
  available.
- **FR-016**: The Render panel MUST provide a camera selector offering
  "Free camera" plus every imported scene camera, with "Free camera" as default.
- **FR-017**: When a scene camera is selected, all camera-derived engine
  uniforms (`view`, `projection`, `MVP`, `uCameraPos`) MUST be derived from that
  camera.
- **FR-017a**: `uCameraPos` MUST always equal the world-space position of the
  currently active camera source. Switching the camera selector (free camera to
  scene camera, scene camera to scene camera, or scene camera back to free
  camera) MUST update `uCameraPos` on the next rendered frame, without shader
  recompilation and without requiring any user interaction with the preview.
- **FR-017b**: When the active scene camera is animated by keyframes,
  `uCameraPos` MUST be re-evaluated every frame from the camera's sampled pose at
  the current playback time.
- **FR-018**: When a scene camera is selected, free-camera orbit/pan/zoom input
  MUST NOT alter the rendered framing, and the free-camera state MUST be
  preserved and restored when the user switches back.
- **FR-019**: Loading a new model MUST re-evaluate the camera selection and MUST
  fall back to "Free camera" when the previous selection no longer exists.
- **FR-020**: A selected scene camera that is targeted by animation channels MUST
  be animated over playback time from those keyframes.
- **FR-021**: Animated camera evaluation MUST use the same playback time and the
  same active clip as the model animation.
- **FR-022**: The application MUST expose a toggleable panel named "Model info"
  from the View menu, following existing ImGui panel/menu conventions.
- **FR-023**: The "Model info" panel MUST display, for the currently loaded
  model: name, source path, mesh count, vertex count, triangle/face count,
  material count, texture presence and per-type texture list, skeleton presence,
  bone count, animation clip count with names and durations, camera count with
  names, and bounding box extents.
- **FR-024**: The "Model info" panel MUST refresh automatically whenever a model
  is loaded or replaced.
- **FR-025**: The "Model info" panel MUST show an explicit empty state when no
  model is loaded and MUST NOT show stale data from a previous model.
- **FR-026**: The "Model info" panel MUST remain read-only and MUST NOT modify
  render, camera, animation, shader, or uniform state.
- **FR-027**: The system MUST update README/help text listing engine-provided
  uniforms so it documents `view` and `projection` as engine-managed and
  explicitly states they are not shown in the Uniforms panel.
- **FR-028**: The system MUST document the animation, camera-selection, and
  "Model info" behavior in the user-facing documentation alongside existing
  panel documentation.
- **FR-029**: System MUST work on the intended supported platforms defined for
  the feature: Windows with the existing GLFW/OpenGL application path.
- **FR-030**: System MUST preserve existing UI patterns, shader editing flows,
  uniform editing behavior, diagnostics behavior, and render interaction unless
  this spec explicitly changes them.
- **FR-031**: System MUST avoid breaking the OpenGL rendering lifecycle or
  leaving the main UI unresponsive during normal use.

### Key Entities *(include if feature involves data)*

- **AnimationClip**: A named set of per-node keyframe channels with a duration in
  seconds; the active clip drives both skeletal/node animation and animated
  cameras.
- **AnimationChannel**: Position, rotation, and scale keyframe sequences for one
  node, sampled independently and interpolated over playback time.
- **SceneCamera**: A camera imported from the model file, with a name, authored
  transform, optional field of view/aspect data, and an optional link to an
  animation channel driving its node.
- **CameraSelection**: The active camera source for the Render preview, either
  the free orbit camera or a scene camera index; determines all camera-derived
  engine uniforms.
- **CameraAutoUniforms**: Per-frame engine-managed matrices `view` and
  `projection`, supplied alongside the existing `MVP`, `model`, and `uCameraPos`
  values and never editable by the user.
- **ModelInfoSummary**: A read-only, per-load snapshot of the imported model's
  statistics presented by the "Model info" panel.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: An imported model with at least one animation clip visibly animates
  in the Render preview when playback runs, and holds a stable pose when paused.
- **SC-002**: A Phoenix shader that computes `projection * view * model` renders
  identically to the same shader computing with `MVP`, for the free camera and
  for a scene camera.
- **SC-003**: Neither `view` nor `projection` appears in the Uniforms panel, and
  both are documented as engine-provided in the README/Shader Help.
- **SC-004**: Switching between "Free camera" and a scene camera changes framing
  within one rendered frame, updates `uCameraPos` to the new camera's world
  position within that same frame, and restores the previous free-camera state on
  switching back.
- **SC-005**: A model with an animated camera shows camera motion over a
  10-second playback run while animated geometry stays synchronized with it.
- **SC-006**: The "Model info" panel shows correct counts for meshes, vertices,
  triangles, materials, textures, bones, animations, and cameras, and updates
  within one model load without reopening the panel.
- **SC-007**: Existing Phoenix auto-uniforms from earlier specs (`t`, `tend`,
  `beat`, `vpWidth`, `vpHeight`, `aspectRatio`, material, bone, and texture
  values) continue to work after this feature.
- **SC-008**: Primary manual validation completes on Windows with no blocking
  OpenGL, GLFW, or ImGui errors.

## Assumptions

- Keyframe data is the animation data already importable from the model file via
  the existing Assimp-based loader; this feature does not introduce a
  ShaderEditor-authored keyframe editor or Phoenix's `Spline`/`.spline` file
  format for section variables.
- `view` and `projection` are exposed as `mat4` uniforms uploaded per draw,
  matching Phoenix's `Model::Draw` behavior and ShaderEditor's existing
  auto-uniform upload model.
- The projection used for engine uniforms is built from the offscreen Render
  preview texture size, consistent with the existing `aspectRatio` variable.
- "Free camera" is the visible label for the existing orbit/pan preview camera,
  equivalent to Phoenix's `CameraNumber < 0` case.
- Scene cameras are read-only: the user selects one but does not edit its
  authored transform or field of view from ShaderEditor.
- "Model info" is the visible panel and View-menu label, and the panel is purely
  informational.
- When a scene camera lacks field-of-view or aspect information, the preview
  falls back to the existing projection parameters rather than failing to render.

## Implementation Notes

Findings from building this feature that were not obvious from the Phoenix
sources and that future work should not have to rediscover.

### Assimp behavior verified against the installed sources

- **`aiCamera::mHorizontalFOV` is the *full* horizontal FOV, not the half
  angle**, contrary to what parts of the documentation suggest. Verified in
  Assimp 6.0.4''s `glTF2Importer.cpp`, which computes it as
  `2*atan(tan(yfov/2)*aspect)`. The correct conversion back is
  `vFov = 2*atan(tan(hFov/2)/aspect)`. Treating it as the half angle produced a
  90 degree FOV where the file authored 45.
- **The aspect used to undo that conversion must be the one Assimp used**, i.e.
  `camera.aspectRatio > 0 ? camera.aspectRatio : 1.0`, not the viewport aspect.
  Assimp substitutes `1.0` when the file declares no aspect ratio.
- **`aiProcess_FindInstances` merges byte-identical meshes** into one mesh
  referenced by several nodes. Any code that assumes one draw per mesh will
  silently drop geometry.

### Rendering invariants

- **Node keyframes and skeletal animation are folded in differently.** Skeletal
  animation is delivered through `gBones`; node-level animation (an object that
  moves without a skeleton) must be folded into each mesh''s `model` matrix from
  its scene node''s animated transform. Skinned models are deliberately excluded
  from the latter, because `gBones` already bakes in the node hierarchy and
  applying it twice would transform those vertices twice.
- **Orbit and pan are camera operations, never model operations.** Baking orbit
  into the model matrix looks identical for the free camera (rotating the world
  equals inversely rotating the camera), but it leaks the free-camera framing
  into object placement, so the rotation stays visible after switching to a
  scene camera and also corrupts the `model` uniform used for normals.
- **Draw one instance per `(mesh, node)` pair, not one per mesh.** Scenes built
  from repeated props reference few meshes from many nodes (the validation scene
  used 7388 node references over 169 meshes); drawing per mesh renders each prop
  once and drops every other copy.
- **Model bounds must be measured with instance placement applied.** Instanced
  copies all sit at the origin in mesh-local space, so a local-space AABB
  measures such a scene far smaller than it is drawn and mis-frames the camera.
- **Scene nodes are stored depth-first**, so a parent always has a lower index
  than its children and world transforms resolve in a single linear pass with no
  recursion.

### Performance

Scenes made of many small meshes are CPU-bound in the draw loop, not GPU-bound.
Three changes were required to keep them interactive:

- Evaluate the whole node hierarchy **once per frame** instead of once per mesh.
  The per-mesh walk was O(meshes x nodes) and allocated on every call; on a
  447-mesh scene it cost 24.78 ms/frame versus 0.048 ms/frame batched.
- **Deduplicate materials at import** and draw meshes grouped by material, so
  material uniforms and textures are bound once per distinct material rather
  than once per mesh.
- **Memoize each texture slot''s GL texture id.** Embedded textures were being
  re-hashed over their entire encoded byte range on every bind, every frame,
  only to look up an already-uploaded texture.
