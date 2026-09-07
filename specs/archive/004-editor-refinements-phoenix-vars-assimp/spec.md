# Feature Specification: Editor Menu Refinements, Phoenix Auto-Uniforms, and Assimp Model Import

**Feature Branch**: `004-editor-refinements-phoenix-vars-assimp`
**Created**: 2026-09-07
**Status**: Implemented and closed (2026-09-07)
**Input**: User description: "Añadir 'Save As' al menú File; renombrar 'Save Current Shaders' a 'Save shader' y 'Update Shaders' a 'Update Shader'; dar soporte a variables uniform automáticas del motor Phoenix (t, tend, beat, ...) rellenadas por el propio ShaderEditor; dar soporte a abrir modelos 3D con Assimp, reproduciendo el naming de texturas y el soporte de animación de la sección drawScene de Phoenix."

**Phoenix Reference Commit**: All Phoenix-specific behavior in this spec
(texture/material uniform naming, vertex attribute names, `gBones` upload,
Assimp post-process flags) is derived from the `Spontz/Phoenix` GitHub
repository at commit
[`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`](https://github.com/Spontz/Phoenix/commit/75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d)
(tag `v4.2.4`, 2026-08-27), specifically:
`Engine/src/sections/drawScene.cpp`, `Engine/src/core/renderer/Mesh.cpp`,
`Engine/src/core/renderer/Material.h`/`.cpp`, `Engine/src/core/renderer/Model.cpp`,
and `Engine/src/core/renderer/ShaderVars.h`. If Phoenix's engine is updated
after this date, compare these files against the new revision and file a
follow-up spec/change for any naming or behavior drift affecting FR-015
through FR-018.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Save the current shader under a new name (Priority: P1)

As a shader author, I want a "Save As" option in the File menu so I can save the
currently open Phoenix `.glsl` shader under a different file name/path without
overwriting the original file, and continue editing the new file afterwards.

**Why this priority**: This is a foundational file-management gap; without it,
users can only overwrite the file they opened, which blocks iterative
experimentation (e.g., save variants of a shader).

**Independent Test**: Open an existing `.glsl` shader, choose File → Save As,
pick a new file name in the native save dialog, confirm a new file is written
with the current editor content, and confirm the editor's active document now
points to the new file (title/path and subsequent Ctrl+S target the new file).

**Acceptance Scenarios**:

1. **Given** a shader is open and has unsaved edits, **When** the user selects
   "Save As" and picks a new path, **Then** a new `.glsl` file is written with
   the current unified source, the dirty flag is cleared, and the workspace's
   active document now references the new path.
2. **Given** "Save As" is invoked, **When** the user cancels the native save
   dialog, **Then** no file is written and the currently open document/path is
   unchanged.
3. **Given** a shader was saved via "Save As" to a new path, **When** the user
   subsequently uses "Save shader" (Ctrl+S) or "Update Shader", **Then** it
   operates against the new path/document, not the original file.
4. **Given** no shader is currently open, **When** the user selects "Save As",
   **Then** the system MUST behave the same way "Save shader" behaves today
   with no document open (no crash, diagnostic message if applicable).

---

### User Story 2 - Renamed menu items match the terminology used elsewhere in the app (Priority: P1)

As a user of the File menu, I want the existing "Save Current Shaders" and
"Update Shaders" menu items renamed to "Save shader" and "Update Shader"
respectively, so the wording matches the single-file (one shader document)
editing model already used throughout the app.

**Why this priority**: Pure terminology fix with no behavior change; trivial
and safe to ship first alongside Save As.

**Independent Test**: Open the File menu and confirm the two items show the
new labels, and confirm both still perform their existing actions (save to
current path, recompile/apply shader).

**Acceptance Scenarios**:

1. **Given** the File menu is open, **When** the user looks at the menu items,
   **Then** they read "Save shader" (Ctrl+S) and "Update Shader" (Ctrl+Enter)
   instead of the previous labels.
2. **Given** the rename, **When** the user activates either item (menu click or
   keyboard shortcut), **Then** the underlying behavior is unchanged from
   today (save current document to its existing path; recompile/apply the
   shader to the preview).

---

### User Story 3 - Phoenix time-based auto-uniforms are available in every shader (Priority: P2)

As a shader author targeting the Phoenix engine, I want ShaderEditor to
automatically declare and fill values for the Phoenix "section timing"
uniforms — starting with `t` (elapsed time), `tend` (section duration), and
`beat` (music beat progression) — so I can preview Phoenix shaders that
reference these uniforms without manually wiring them up, matching how
Phoenix's own engine feeds these values at runtime.

**Why this priority**: Enables previewing a large class of real Phoenix
shaders (time/beat-driven effects) that are currently unusable in
ShaderEditor because these uniforms are always zero/undefined.

**Independent Test**: Open (or write) a Phoenix shader that declares
`uniform float t;`, `uniform float tend;`, and `uniform float beat;`, run
"Update Shader", and confirm the Render View shows the shader animating in
real time; confirm Play/Pause/Reset and the `tend`/BPM controls affect the
visible output as described in the acceptance scenarios below.

**Acceptance Scenarios**:

1. **Given** a shader that declares `uniform float t;`, **When** the user
   presses Play in the Render View / preview controls, **Then** `t` increases
   continuously in seconds from zero (or from the last Reset), matching
   elapsed wall-clock playback time.
2. **Given** playback is running, **When** the user presses Pause, **Then**
   `t` (and `beat`) stop advancing and hold their current value; **When** the
   user presses Play again, **Then** they resume advancing from where they
   left off.
3. **Given** playback is running or paused, **When** the user presses Reset,
   **Then** `t` returns to 0 and `beat` returns to its value at `t=0`.
4. **Given** a numeric "Section duration" (`tend`) field in the UI, **When**
   the user edits its value, **Then** the `tend` uniform passed to the shader
   updates to exactly that value on the next frame, independent of playback
   state.
5. **Given** a numeric "BPM" field in the UI, **When** the user edits its
   value, **Then** `beat` is computed automatically each frame as
   `beat = t * bpm / 60` (beats elapsed since `t=0`) using the current `t` and
   the current BPM value, with no separate manual control for `beat` itself.
6. **Given** a shader that does NOT declare one or more of `t`, `tend`, or
   `beat`, **When** the shader is compiled and run, **Then** the system MUST
   NOT produce a warning/error for the absence of those uniforms (they are
   optional, supplied only when declared) and existing manually-defined
   uniforms of the same name continue to take precedence in line with FR-013.
7. **Given** the Diagnostics/Uniforms panel, **When** a shader uses any of
   these auto-supplied uniforms, **Then** the panel indicates that the
   uniform's value is automatically managed by ShaderEditor rather than
   user-editable like other uniforms.

---

### User Story 4 - Import a 3D model with Assimp and preview it with the current shader (Priority: P2)

As a shader author, I want to open a 3D model file (e.g., `.fbx`, `.gltf`,
`.obj`, `.dae`) via Assimp and have it replace the preview primitive in the
Render View, with per-mesh textures bound and named exactly like Phoenix's
`drawScene` section, and with any embedded skeletal animation played back
automatically, so I can see how my shader looks on real production assets
instead of only built-in primitives.

**Why this priority**: Large, valuable capability, but depends on User Story 3
being conceptually consistent (both feed automatic engine-like values to the
shader) and is more implementation-heavy; sequenced after the smaller stories.

**Independent Test**: Use the existing "Open" flow (or a new "Open Model..."
entry) to load a rigged, textured, animated sample model file, confirm it
appears in the Render View instead of the built-in preview primitive, confirm
its diffuse/normal/etc. textures render using the current shader when the
shader samples uniforms named `texture_diffuse1`, `texture_normal1`, etc., and
confirm the animation plays back over time using the same bone-matrix uniform
name Phoenix uses (`gBones`).

**Acceptance Scenarios**:

1. **Given** the Render View is showing the default preview primitive,
   **When** the user opens a valid model file supported by Assimp, **Then**
   the model's mesh(es) replace the primitive as the render target for the
   active shader, keeping the rest of the editor (shader source, uniforms
   panel) unchanged.
2. **Given** a model with a material that has a diffuse texture, **When** the
   model is loaded, **Then** the texture is bound to a sampler2D uniform named
   `texture_diffuse1` in the active shader (and `texture_diffuse2`, etc. for
   additional diffuse textures on the same material), matching Phoenix's
   `texture_<type>N` naming (`texture_diffuse`, `texture_specular`,
   `texture_normal`, `texture_height`, `texture_ambient`, `texture_emissive`,
   as produced by Phoenix's `Material::loadTextures`).
3. **Given** a model with an embedded skeletal animation, **When** the model
   is loaded and playback is running (per User Story 3's Play/Pause/Reset
   controls), **Then** bone transforms are computed per frame and uploaded to
   a `mat4[]` uniform named `gBones` exactly as Phoenix's `Model::Draw` does,
   and the shader's vertex stage can use `aBoneID`/`aBoneWeight` vertex
   attributes (Phoenix naming) to apply skinning if it implements it.
4. **Given** a model has no animation data, **When** it is loaded, **Then** it
   renders as a static mesh with identity bone transforms (or no `gBones`
   upload) with no errors.
5. **Given** a model file Assimp cannot parse or that references missing
   texture files, **When** the user attempts to open it, **Then** the system
   reports a diagnostic identifying the problem (matching the existing
   diagnostics flow) without crashing and without leaving the Render View in
   an inconsistent/blank state (previous primitive/model remains visible).
6. **Given** a model is loaded, **When** the user opens a new shader file
   (not a new model), **Then** the currently loaded model remains the render
   target and the new shader is applied to it, consistent with User Story 1
   of spec 003 (shader source and render target are independent concerns).
7. **Given** a model is loaded, **When** the user selects a built-in preview
   primitive (quad/cube/sphere/etc.) from the existing primitive selector,
   **Then** the Render View switches back to rendering that primitive with
   the active shader, and the previously loaded model is kept in memory
   (not discarded) so selecting "model" as the render target again shows it
   without re-importing.

---

### User Story 5 - Bundled sample assets prove animation and bump mapping work out of the box (Priority: P2)

As a shader author trying ShaderEditor for the first time, I want the app to
ship with a ready-to-use example shader that animates an Assimp-imported
skinned model, another example shader that demonstrates bump mapping, and a
third example shader that combines skeletal animation with PBR
(metallic-roughness) shading, plus small, royalty-free, textured and animated
sample 3D models to exercise them, so I can verify User Stories 3 and 4 work
correctly without having to source my own models or write skinning/bump/PBR
shaders from scratch first.

**Why this priority**: Without bundled examples, User Stories 3 and 4 are
difficult to verify or demo — a user (or reviewer) would need to find their
own compatible model and write compatible shaders first. This story is the
concrete, runnable proof that the rest of the feature works.

**Independent Test**: Fresh checkout/build of the app; without downloading
anything else, use File → Open Shader... to open the bundled bone-animation
example shader and File → Open Model... to open the bundled sample model;
confirm the model renders textured and animated. Separately open the bundled
bump-mapping example shader against the same (or a primitive) target and
confirm visible surface relief. Separately open the bundled PBR-animation
example shader against the bundled PBR sample model and confirm it renders
textured, animated, and with metallic/roughness-driven shading distinct from
the plain diffuse lighting used elsewhere.

**Acceptance Scenarios**:

1. **Given** a fresh build of ShaderEditor, **When** the user browses the
   runtime assets shaders folder, **Then** at least one example shader
   declares and uses the bone/skinning attributes and the `gBones` uniform
   from User Story 4 (FR-017/FR-018), and at least one other example shader
   implements a bump/normal-mapping lighting technique.
2. **Given** a fresh build of ShaderEditor, **When** the user browses the
   runtime assets models folder, **Then** at least one small (order of a few
   hundred KB or less), textured, skeletally-animated 3D model file is
   present in a format Assimp/the app's Open Model dialog supports, ideally
   `.glb`.
3. **Given** the bundled sample model and the bundled bone-animation example
   shader, **When** the user opens the model then applies the shader (or vice
   versa) and presses Play, **Then** the model renders with its texture
   mapped and its animation visibly playing, using the exact Phoenix uniform
   names from FR-015–FR-018.
4. **Given** the bundled bump-mapping example shader, **When** it is applied
   to the bundled sample model or a built-in primitive, **Then** the rendered
   surface visibly shows relief/shading detail beyond flat per-pixel diffuse
   lighting (e.g., via a normal map texture or a procedurally derived bump
   from an existing texture, since not every sample asset ships a dedicated
   normal map).
5. **Given** the bundled sample model's license, **When** it is redistributed
   inside this repository's assets, **Then** it MUST be royalty-free for this
   use and any required attribution MUST be included alongside the asset
   (e.g., a README next to the model file).
6. **Given** the bundled PBR sample model and the bundled PBR-animation
   example shader, **When** the user opens the model then applies the shader
   and presses Play, **Then** the model renders textured and animated, and
   its shading responds to metallic/roughness input (either dedicated
   metalness/roughness textures or scalar factors) in a way visibly different
   from the non-PBR lighting used by the other example shaders.
7. **Given** the bundled PBR sample model's license, **When** it is
   redistributed inside this repository's assets, **Then** it MUST be
   royalty-free for this use and any required attribution MUST be included
   alongside the asset.

### Edge Cases

- What happens when the user does "Save As" over a file that already exists
  and is currently open elsewhere or read-only? (Should surface the same kind
  of diagnostic/error the existing save path uses for write failures.)
- What happens when BPM is set to 0 or a negative value? Per FR-010, `beat`
  MUST be held at 0 in that case rather than producing NaN/Inf. For positive
  BPM, `beat` is the normalized phase in `[0, 1)`, wrapping at each beat.
- What happens when `tend` is 0? Since `t`/`beat` do not divide by `tend` in
  this spec (that ratio, if any, is left to the shader author), no special
  handling beyond passing the literal value is required in this iteration.
- What happens when a model file contains more bones than a single mesh's
  `gBones` array can support in one draw call? The system MUST apply
  Assimp's `aiProcess_SplitByBoneCount` post-process step during import, so
  oversized meshes are automatically split into multiple sub-meshes that each
  fit within the supported bone limit, matching Phoenix's own import pipeline
  (`Model.cpp`'s post-process flags).
- What happens on each supported platform (Windows-only today) when the
  Assimp/model file or its textures are not available or permissions are
  denied?
- What happens when a PBR model's glTF material has no dedicated
  metalness/roughness texture (only scalar factors)? The bundled PBR example
  shader MUST fall back to the scalar `metallicFactor`/`roughnessFactor`
  uniforms rather than failing or rendering as fully black/white.
- Could this feature break OpenGL context setup, shader/resource loading, or
  UI responsiveness during normal use? Loading large animated models must not
  block the render loop; long imports should not freeze the UI thread beyond
  a brief, acceptable load time.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The File menu MUST include a "Save As" item that opens a native
  save-file dialog (matching the existing "Open Shader..." dialog style)
  filtered to `.glsl` files.
- **FR-002**: Confirming "Save As" MUST write the current unified shader
  document's full source to the chosen path, clear the dirty flag, and update
  the active document's path so subsequent "Save shader" and title/diagnostics
  references use the new path.
- **FR-003**: Cancelling the "Save As" dialog MUST leave the current document,
  path, and dirty flag unchanged.
- **FR-004**: The File menu item previously labeled "Save Current Shaders"
  MUST be relabeled "Save shader" with no change to its Ctrl+S behavior.
- **FR-005**: The File menu item previously labeled "Update Shaders" MUST be
  relabeled "Update Shader" with no change to its Ctrl+Enter behavior.
- **FR-006**: The system MUST automatically supply a value for a uniform named
  `t` (type float) when a shader declares it, equal to elapsed playback time in
  seconds since the last Reset, advancing only while playback is not paused.
- **FR-007**: The system MUST provide Play, Pause, and Reset controls in the
  UI that govern the advancement of `t` (and, transitively, `beat`).
- **FR-008**: The system MUST provide an editable numeric "Section duration"
  control in the UI whose value is automatically supplied to any shader that
  declares a uniform named `tend` (type float), independent of Play/Pause
  state.
- **FR-009**: The system MUST provide an editable numeric "BPM" control in the
  UI and MUST automatically supply a uniform named `beat` (type float) equal
  to the fractional part of `t * bpm / 60` to any shader that declares it,
  recomputed every frame from the current `t` and BPM values. Therefore
  `beat` is always in `[0, 1)` for positive BPM and wraps at each beat.
- **FR-010**: The system MUST guard the `beat` computation so that a BPM of
  zero or a negative value never produces NaN/Inf: when BPM <= 0, `beat` MUST
  be held at 0 (no progression) instead of being computed from `t * bpm / 60`.
- **FR-011**: Auto-supplied uniforms (`t`, `tend`, `beat`) MUST only be
  written to the shader when it actually declares a uniform with that exact
  name and a compatible scalar float type; shaders that omit any of these
  names MUST compile and run without warnings related to their absence.
- **FR-012**: The Uniforms panel MUST visually distinguish auto-supplied
  uniforms (`t`, `tend`, `beat`) from user-editable uniforms, and MUST NOT
  expose manual editing controls for `t` or `beat` (their values are
  computed); the `tend`/BPM controls live with the playback controls per
  FR-007–FR-009, not as generic uniform editors.
- **FR-013**: If a shader declares any of `t`, `tend`, or `beat` with an
  incompatible type (e.g., `vec3`), the system MUST treat it like any other
  type mismatch in the existing uniform-handling flow (diagnostic, no crash)
  rather than attempting to force the automatic value.
- **FR-014**: The system MUST allow opening a 3D model file through Assimp
  (in addition to existing shader/image open flows) and use it as the active
  render target in place of the built-in preview primitive.
- **FR-015**: For each imported model material, the system MUST bind textures
  to sampler2D uniforms using the same `texture_<type><N>` naming Phoenix uses
  in `Material::loadTextures`/`Mesh::setMaterialShaderVars`: `texture_diffuse`,
  `texture_specular`, `texture_ambient`, `texture_height`, `texture_normal`,
  `texture_emissive`, `texture_roughness`, `texture_shininess`,
  `texture_ambientoclussion`, `texture_metalness`, `texture_unknown`,
  `texture_none`, each suffixed with a 1-based index per texture of that type
  on the material (e.g., `texture_diffuse1`, `texture_diffuse2`).
- **FR-016**: The system MUST also expose the material scalar/color
  properties Phoenix uploads alongside textures — `Mat_Ka` (ambient color),
  `Mat_Kd` (diffuse color), `Mat_Ks` (specular color), `Mat_KsStrenght`
  (specular strength) — as automatically supplied uniforms when a shader
  declares them, using those exact names.
- **FR-017**: For models containing skeletal animation, the system MUST
  compute per-frame bone transforms and upload them to a `mat4[]` uniform
  named `gBones`, matching Phoenix's `Model::Draw`, and MUST play the
  animation forward using the same Play/Pause/Reset transport as `t` (FR-007).
- **FR-018**: The system MUST expose per-vertex bone attributes using
  Phoenix's naming/layout (`aBoneID` as an unsigned int4, `aBoneWeight` as a
  float4, alongside `aPos`, `aNormal`, `aTexCoords`, `aTangent`,
  `aBiTangent`) so shaders that implement skinning can consume them exactly
  as Phoenix shaders do.
- **FR-019**: If a model file fails to load (unsupported/corrupt format,
  missing referenced texture), the system MUST surface a diagnostic through
  the existing Diagnostics panel and MUST leave the previously active render
  target (primitive or prior model) visible and unaffected.
- **FR-020**: Loading a new shader file while a model is loaded MUST NOT
  unload the model; the shader is simply re-applied to the current render
  target (model or primitive), consistent with existing shader/render-target
  independence.
- **FR-021**: System MUST work on the intended supported platforms defined
  for the feature (Windows, matching the existing native-dialog constraint
  already documented for Open/Save flows).
- **FR-022**: System MUST preserve existing UI patterns (native dialogs,
  Diagnostics panel, Uniforms panel conventions) unless this spec defines an
  approved change.
- **FR-023**: System MUST avoid breaking the OpenGL rendering lifecycle or
  leaving the main UI unresponsive during normal use, including while loading
  large animated models.
- **FR-024**: The repository's runtime shader assets (`assets/shaders/`)
  MUST include at least one example shader demonstrating skeletal animation
  via the `gBones` uniform and `aBoneID`/`aBoneWeight` vertex attributes
  (FR-017/FR-018), and at least one example shader demonstrating bump/normal
  mapping (tangent-space normal map sampling, or a procedurally derived bump
  when no dedicated normal map asset is bundled).
- **FR-025**: The repository's runtime assets MUST include at least one
  small, textured, skeletally-animated 3D model file (target: a few hundred
  KB or less, `.glb` preferred) that is royalty-free for redistribution in
  this repository, with any required attribution recorded alongside the
  asset (e.g., a README in the same folder).
- **FR-026**: The bundled example shaders and the bundled sample model MUST
  be usable together to manually verify User Stories 3 and 4 end-to-end
  (textured + animated rendering) without requiring the user to source any
  additional files.
- **FR-027**: The repository's runtime assets MUST include at least one
  example shader that combines skeletal animation (the same `gBones`/
  `aBoneID`/`aBoneWeight` attributes from FR-017/FR-018) with a
  metallic-roughness PBR lighting model (using dedicated
  metalness/roughness textures when present, falling back to scalar
  `metallicFactor`/`roughnessFactor` uniforms otherwise).
- **FR-028**: The repository's runtime assets MUST include at least one
  small, textured, skeletally-animated 3D model file whose material uses
  glTF's `pbrMetallicRoughness` model (target: under 1 MB, `.glb` preferred)
  that is royalty-free for redistribution in this repository, with any
  required attribution recorded alongside the asset.
- **FR-029**: Engine-owned uniforms `MVP`, `model`, and `uCameraPos` MUST be
  uploaded automatically and excluded from the editable Uniforms panel. The
  Shader Help panel MUST document all engine-owned uniforms, material texture
  naming, and Phoenix-compatible vertex attributes.
- **FR-030**: The Render panel MUST provide an `Open Model...` action, an
  animation clip selector for loaded models (including a bind-pose option),
  and model selection without re-importing.
- **FR-031**: Imported textures MUST support both external files and embedded
  encoded image data in formats such as `.glb`.
- **FR-032**: Runtime assets MUST include an animated material-only shader that
  uses `Mat_Ka`, `Mat_Kd`, `Mat_Ks`, and `Mat_KsStrenght` without texture
  sampling.

### Key Entities *(include if feature involves data)*

- **Playback Clock**: Represents the Play/Pause/Reset state and elapsed time
  `t` shared by time-based auto-uniforms and model animation playback.
- **Section Timing Settings**: User-editable `tend` (section duration) and
  `bpm` values used to derive `tend` and `beat` uniforms.
- **Auto-Uniform**: A named, typed uniform (`t`, `tend`, `beat`, `Mat_Ka`,
  `Mat_Kd`, `Mat_Ks`, `Mat_KsStrenght`, `gBones`) whose value is computed by
  ShaderEditor rather than entered by the user, distinguished in the Uniforms
  panel from user-editable uniforms.
- **Imported Model**: A render target loaded via Assimp, with one or more
  meshes, each carrying a material (textures + color/scalar properties) and,
  optionally, a bone hierarchy and animation clips, replacing the built-in
  preview primitive as the shader's render target.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Users can save the active shader to a brand-new file name via
  "Save As" in under 3 clicks/interactions (menu → dialog → confirm), with the
  editor correctly tracking the new file afterward.
- **SC-002**: 100% of existing "Save"/"Update" keyboard shortcuts and behavior
  continue to work unchanged after the menu label rename.
- **SC-003**: A Phoenix shader using `t`, `tend`, and `beat` renders visibly
  animated output within one frame of pressing Play, with Pause freezing the
  visual state and Reset returning it to the `t=0` appearance.
- **SC-004**: Editing the BPM or section-duration fields updates the rendered
  output on the next frame with no need to reopen or re-save the shader.
- **SC-005**: The bundled sample model (`assets/models/Fox/Fox.glb`, a small
  rigged, textured, animated fox with Walk/Run/Survey animation clips) loads,
  displays correctly textured, and animates when previewed with the bundled
  `assets/shaders/bone_animation.glsl` example shader, with texture and bone
  uniform names matching Phoenix's own drawScene output byte-for-byte (same
  uniform name strings).
- **SC-006**: The Render View keeps a valid render loop and remains responsive
  (no UI freeze beyond a brief, expected load time) while importing the
  bundled sample model (~159 KB) or any larger sample model exercised in
  testing.
- **SC-007**: The bundled `assets/shaders/bump_mapping.glsl` example shader
  produces visibly different, relief-shaded output compared to the existing
  flat-lit `pixel_lighting.glsl` example when applied to the same primitive
  or model.
- **SC-008**: The bundled `assets/models/CesiumMan/CesiumMan.glb` sample model
  (a small, textured, skeletally-animated character with a
  `pbrMetallicRoughness` material) loads, displays correctly textured, and
  animates when previewed with the bundled
  `assets/shaders/pbr_animation.glsl` example shader, producing visibly
  different shading than the non-PBR example shaders when the light or
  camera position changes.

## Assumptions

- Native file dialogs remain Windows-only, consistent with the existing
  "Open Shader..."/"Open Image..." implementation; Assimp model import uses
  the same native dialog approach and is likewise Windows-only in this
  iteration.
- Only `t`, `tend`, and `beat` are in scope for automatic Phoenix uniforms in
  this spec; other Phoenix engine variables (e.g., resolution, frame counter,
  mouse, delta time, audio-reactive values) are explicitly out of scope and
  will be addressed in future iterations, one at a time, as follow-up specs.
- `beat` has no real audio/BPM-detection engine behind it; it is purely
  derived from elapsed time and a user-entered BPM value
  (`beat = fract(t * bpm / 60)` for positive BPM), not from an actual audio
  track. It is always normalized to `[0, 1)`.
- The Assimp import scope for this spec is limited to loading a single model
  as the active render target with textures and skeletal animation, mirroring
  Phoenix's `drawScene` section; it does not include multi-model scenes,
  camera import, light import, or saving/exporting models.
- The same single active shader (`#type vertex` / `#type fragment` unified
  document) is applied to the imported model's mesh(es); no separate
  model-specific shader pipeline is introduced.
- Switching back to a built-in preview primitive after a model is loaded
  reuses the existing primitive selector; the model stays loaded in memory
  and can be re-selected as the render target without re-importing.
- The bundled sample model is
  [Fox](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/Fox)
  from the Khronos `glTF-Sample-Assets` repository (`Fox.glb`, ~159 KB),
  licensed CC0 (model) plus CC-BY 4.0 (rigging/animation and glTF conversion);
  it is royalty-free and redistributable in this repository provided the
  CC-BY attribution is kept alongside the asset (see
  `assets/models/Fox/README.md`). It does not ship a dedicated normal map, so
  the bundled bump-mapping example shader also supports a procedurally
  derived bump technique as a fallback, per FR-024.
- The bundled PBR sample model is
  [Cesium Man](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/CesiumMan)
  from the Khronos `glTF-Sample-Assets` repository (`CesiumMan.glb`, ~438 KB),
  licensed CC-BY 4.0; it is royalty-free and redistributable in this
  repository provided the CC-BY attribution is kept alongside the asset (see
  `assets/models/CesiumMan/README.md`).

## Closure

This specification is implemented and closed on 2026-09-07. The final
implementation is covered by commits `edb5622`, `7118a87`, `36dd9c0`,
`0de028a`, `492dc73`, `c889b8e`, and `ec87b0f` on
`004-editor-refinements-phoenix-vars-assimp`.

Debug and Release builds succeeded and the complete CTest suite passed. The
README, Shader Help panel, research notes, data model, contracts, and
quickstart have been synchronized with the shipped behavior. Any future
Phoenix naming or behavior changes should be handled in a new follow-up spec
against reference commit
`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`.
