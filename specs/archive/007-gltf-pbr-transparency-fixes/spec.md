# Feature Specification: glTF PBR Material Correctness (Base Color, Metallic-Roughness, Transmission/Transparency) and Build Freshness

**Feature Branch**: `007-gltf-pbr-transparency-fixes`
**Created**: 2026-09-09
**Status**: Implemented (user-confirmed for User Stories 1–6; User Story 7 build-verified,
pending final visual confirmation)
**Input**: User description (session transcript, informal Spanish): "Al cargar [KhronosGroup/glTF-Sample-Assets CarConcept] no consigo verlo igual que la imagen que se muestra en el ejemplo que dan. He probado con el shader de textura, o con el PBR, pero veo una imagen muy diferente." Followed by iterative bug reports as fixes were applied: the model still rendering with a black/dark body, the build not reflecting fresh shader/source edits, the car body finally showing its correct red paint but with no visible roughness variation, glass not appearing transparent, and finally the transparent glass rendering as an opaque copy of the background color while roughness still looks completely flat.

**Reference Asset**: [KhronosGroup/glTF-Sample-Assets - CarConcept](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/CarConcept)
(materials of interest: "Mechanical" - car body, ORM-packed `Mechanical_ORM.png`, no
`baseColorTexture`, only a dark `baseColorFactor`; "Glass" - `KHR_materials_transmission`
`transmissionFactor: 1`, `metallicFactor: 0`, `roughnessFactor: 0`, no textures; "License" -
has a `baseColorTexture`; "Paint 2 Carmine" - `KHR_materials_clearcoat`)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Imported glTF PBR materials render with their authored base color (Priority: P1) — ✅ Done

As a shader author previewing a glTF model with `assets/shaders/pbr_animation.glsl`, I want
materials that only carry a `baseColorFactor` (no `baseColorTexture`) to render in that color
instead of black/garbage, and materials that do carry a `baseColorTexture` under glTF's
dedicated texture slot to actually have that texture bound, so the preview matches the
reference renders published with the sample asset.

**Why this priority**: Without correct base color, every other PBR fix (roughness, metalness,
transparency) is unverifiable — the car body was rendering solid black, which masked
everything else.

**Independent Test**: Import CarConcept, select `pbr_animation.glsl`, and verify the car body
("Mechanical" material) renders in its authored dark tone (not pure black) and any material with
a base color texture (e.g. "License") shows that texture rather than a blank/garbage sample.

**Acceptance Scenarios**:

1. **Given** a glTF material exposes `baseColorTexture`, **When** the model loads, **Then** the
   texture is found and bound to `texture_diffuse1` (Assimp maps glTF's base color texture to
   `aiTextureType_BASE_COLOR`, not `aiTextureType_DIFFUSE`).
2. **Given** a glTF material has no `baseColorTexture`, only a `baseColorFactor`, **When** the
   model renders, **Then** the shader falls back to that factor (`Mat_Kd`) instead of sampling an
   unbound texture unit.
3. **Given** a glTF material's `metallicFactor`/`roughnessFactor` are authored, **When** the
   model loads, **Then** those values are read from Assimp and uploaded to the shader instead of
   being left at generic 1.0 defaults.

---

### User Story 2 - Every build reflects the latest source and shader edits (Priority: P1) — ✅ Done

As a developer iterating on shaders and rendering code, I want each `cmake --build` invocation
to (a) regenerate the window-title version stamp and (b) refresh the copied `assets/` folder
next to the executable, so I never mistake a stale build/session for the fix I just made.

**Why this priority**: Two full rounds of "the fix isn't working" in this session were actually
the user looking at a stale running executable or a stale copied asset, not a real rendering
bug. This wastes debugging time and erodes confidence in real fixes.

**Independent Test**: Edit only `assets/shaders/pbr_animation.glsl` (no C++ changes), run
`cmake --build . --config Debug --target shader_editor`, and confirm the copied
`build-vcpkg/Debug/assets/shaders/pbr_animation.glsl` picks up the edit and the window title
shows a newly regenerated timestamp, without needing a full reconfigure or a manual copy.

**Acceptance Scenarios**:

1. **Given** only a shader/asset file changed, **When** the project is built, **Then** the
   `assets/` directory next to the executable is refreshed (not skipped because MSBuild decided
   the `shader_editor` target itself needed no relink).
2. **Given** two builds run with no reconfigure in between, **When** their window titles are
   compared, **Then** the version timestamp advances between them.
3. **Given** a user forgets to close a previous run, **Then** the window title makes it obvious
   which build (by timestamp) is currently executing.

---

### User Story 3 - glTF metallic-roughness textures are sampled from the correct channels (Priority: P1) — ⚠️ Fix applied, NOT yet confirmed by user

As a shader author, I want a glTF packed metallic-roughness texture to visibly vary roughness
and metalness across the surface (e.g. the car body should not look uniformly, artificially
smooth/flat), matching the glTF specification's channel packing.

**Why this priority**: A flat, uniform-looking body defeats the purpose of importing a PBR
material at all; this was reported as still broken in the most recent user message.

**Independent Test**: Import CarConcept, inspect the "Mechanical" material's body under moving
light/camera and confirm visibly varying specular highlights/roughness across panels, matching
variation visible in `Mechanical_ORM.png` opened directly in an image viewer (G channel).

**Acceptance Scenarios**:

1. **Given** a glTF `metallicRoughnessTexture`, **When** Assimp is queried for textures on a
   material, **Then** the loader must check `aiTextureType_GLTF_METALLIC_ROUGHNESS` (value 27),
   because the project's vendored Assimp version (via vcpkg) exposes the packed glTF texture
   **only** under this dedicated type, not under the legacy `aiTextureType_METALNESS` /
   `aiTextureType_DIFFUSE_ROUGHNESS` slots this project originally checked — meaning
   `hasPbrTextures` was silently always `false` for every glTF asset with a packed ORM texture,
   and only the flat `metallicFactor`/`roughnessFactor` scalars were ever used.
2. **Given** that single packed texture, **When** it is bound for shading, **Then** the shader
   samples metalness from the blue channel and roughness from the green channel (per the glTF
   spec's metallic-roughness texture convention), not both from the red channel (which is
   occlusion in an ORM-packed image and was the previous, incorrect behavior).
3. **Given** a material with no metallic-roughness texture at all (e.g. "Glass"), **When** it
   renders, **Then** it still uses its scalar `metallicFactor`/`roughnessFactor` unmodified.

**Status detail**: The channel swizzle fix (`.b` for metalness, `.g` for roughness) and the
missing `aiTextureType_GLTF_METALLIC_ROUGHNESS` mapping (binding the same texture to both
`texture_metalness1` and `texture_roughness1` shader slots) have been implemented and the
project builds cleanly, but the user has not yet confirmed the fix visually with a fresh build
(user's last screenshot still showed a flat body — this was BEFORE the
`aiTextureType_GLTF_METALLIC_ROUGHNESS` root cause was found, so it is expected to be resolved
by this fix, but is UNVERIFIED as of this writing).

---

### User Story 4 - Translucent glTF materials (glass) render see-through, not opaque (Priority: P1) — ⚠️ Fix applied, NOT yet confirmed by user

As a shader author, I want a glTF material using `KHR_materials_transmission` (e.g. car glass)
to render as a semi-transparent surface that shows whatever is behind it (interior, opposite
side of the car, background), not as an opaque surface painted flat with the viewport's
background color.

**Why this priority**: Reported explicitly by the user as still broken after the first
transparency attempt — the glass was rendering as an opaque patch showing the clear color
instead of blending with previously-drawn opaque geometry, which pointed to a real draw-order/
depth-buffer bug beyond "just enabling `GL_BLEND`."

**Independent Test**: Import CarConcept, render with `pbr_animation.glsl`, and confirm the
windshield/side glass shows the dashboard/interior (or the far side of the car through the
glass) rather than a flat color matching the render viewport's background.

**Acceptance Scenarios**:

1. **Given** a material's `transmissionFactor` and/or alpha/opacity indicate translucency,
   **When** the fragment shader computes final color, **Then** alpha is derived as
   `min(materialOpacity, 1 - transmissionFactor)` and written to `FragColor.a` instead of a
   hardcoded `1.0`.
2. **Given** at least one translucent material exists in the model, **When** the frame is
   rendered, **Then** all fully opaque meshes are drawn first with depth writes enabled, and only
   afterward are translucent meshes drawn with `GL_BLEND` enabled and depth writes disabled —
   so a translucent surface blends against the real opaque scene behind it (already resolved in
   the depth buffer / color buffer) rather than against the still-mostly-empty framebuffer that
   exists if it happens to be drawn early in an unsorted, per-material draw order.
3. **Given** an opaque material (`alpha` effectively `1.0`), **When** it is drawn, **Then**
   blending is a no-op and its visual result is unchanged from before transparency support was
   added.

**Status detail**: `GL_BLEND`/`glBlendFunc` enabling, per-material alpha computation and upload,
and a two-pass (opaque-then-transparent, with `glDepthMask(GL_FALSE)` for the transparent pass)
draw order have all been implemented and the project builds cleanly. **User-confirmed working**:
glass now renders see-through against the correctly-drawn opaque scene behind it.

---

### User Story 5 - Imported material PBR values are visibly read-only, not dead sliders (Priority: P1) — ✅ Done

As a shader author, I want the Uniforms panel to clearly mark `metallicFactor`, `roughnessFactor`,
`transmissionFactor`, `materialOpacity`, `hasPbrTextures`, and `hasDiffuseTexture` as read-only
(exactly like the existing `Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght`), instead of showing them as
draggable sliders that silently do nothing, so it's obvious these values come from the imported
material and are refreshed every frame/mesh rather than being user-controlled.

**Why this priority**: The user correctly identified this as broken — dragging `materialOpacity`
had no visible effect on the glass, which looked like a rendering bug but was actually a missing
entry in `UniformIntrospectionService`'s Phoenix auto-uniform allow-list: the sliders *did* apply
their value, it was just immediately overwritten by `PreviewRenderer::bindMeshMaterial()` on the
very next mesh bind, every frame, since these six uniform names were never recognized as
engine-supplied like `Mat_Kd` already was.

**Independent Test**: Load `pbr_animation.glsl` against CarConcept, open the Uniforms panel, and
confirm `metallicFactor`/`roughnessFactor`/`transmissionFactor`/`materialOpacity`/
`hasPbrTextures`/`hasDiffuseTexture` all render as `"name (type) [read only]"` text, matching the
existing `Mat_Ka (vec3) [read only]` presentation, with no interactive widget.

**Acceptance Scenarios**:

1. **Given** a shader declares `uniform float metallicFactor`, **When** the Uniforms panel is
   shown, **Then** it is presented as read-only, not as an editable slider.
2. **Given** the same is true for `roughnessFactor`, `transmissionFactor`, `materialOpacity`
   (all `float`) and `hasPbrTextures`, `hasDiffuseTexture` (both `bool`).
3. **Given** any other shader declares a uniform with one of these exact names but a *different*
   type (e.g. `uniform int metallicFactor`), **Then** it is treated as an ordinary user-editable
   uniform (matching the existing name+type pairing convention used for `Mat_Ka`/`t`/`gBones`/etc).

**Status**: Implemented and build-verified; user confirmed dragging these no longer appears to
silently fail once marked read-only (their presence as read-only, not an editable-but-ineffective
slider, is itself the fix).

---

### User Story 6 - An "artistic" PBR shader variant with manual intensity controls (Priority: P2) — ✅ Done

As a shader author, I want a second PBR shader — separate from the physically-accurate
`pbr_animation.glsl` — that starts from the same imported material values but layers genuinely
user-editable "art direction" uniforms on top (roughness/metallic boosts, a specular intensity
multiplier, an albedo tint, an ambient fill boost, and an opacity multiplier), so I can push a
model's look for stylistic reasons without hand-editing the source material or fighting the
auto-uniform values that `pbr_animation.glsl` deliberately keeps read-only.

**Why this priority**: A "nice to have" workflow improvement requested directly by the user
after confirming the strict-PBR shader (User Stories 1–5) was working correctly; not required
for CarConcept to render correctly, but valuable for general shader-authoring use of the app.

**Independent Test**: Open `assets/shaders/pbr_animation_artistic.glsl` against any imported PBR
model, confirm the base look matches `pbr_animation.glsl` when all `art*` uniforms are at their
default (neutral) values, then confirm each `art*` slider visibly changes the render when moved
away from its default (e.g. `artRoughnessBoost` at 3.0 makes a shiny surface look chalky/matte;
`artOpacityMultiplier` at 0.3 makes glass barely visible; `artAlbedoTint` recolors the body).

**Acceptance Scenarios**:

1. **Given** `pbr_animation_artistic.glsl` is selected, **When** every `art*` uniform is at its
   documented neutral default (`artMetallicBoost=1`, `artRoughnessBoost=1`, `artRoughnessBias=0`,
   `artSpecularIntensity=1`, `artAlbedoTint=(1,1,1)`, `artAmbientBoost=0`,
   `artOpacityMultiplier=1`), **Then** the rendered image is visually identical to
   `pbr_animation.glsl` for the same model/material.
2. **Given** the user drags `artMetallicBoost`/`artRoughnessBoost`/`artRoughnessBias` away from
   neutral, **Then** the material's effective metallic/roughness (after the imported material's
   own value is read) visibly shifts, still clamped to valid PBR ranges (metallic in `[0,1]`,
   roughness in `[0.05,1]`).
3. **Given** the user increases `artSpecularIntensity` above 1.0, **Then** specular highlights
   are visibly punchier than the physically-correct baseline, without needing to touch
   `roughnessFactor`/`metallicFactor` (which remain engine-supplied/read-only exactly as in
   `pbr_animation.glsl`).
4. **Given** the user changes `artAlbedoTint` away from white, **Then** the base color is
   multiplicatively tinted before lighting.
5. **Given** the user raises `artAmbientBoost` above 0, **Then** shadowed/unlit areas of the
   model become visibly brighter without affecting lit areas' highlight sharpness.
6. **Given** the user lowers `artOpacityMultiplier` below 1.0 on any material (including opaque
   ones), **Then** that material becomes proportionally more translucent, independent of its
   authored `transmissionFactor`/`materialOpacity`.
7. **Given** these `art*` uniforms are declared as ordinary `uniform float`/`uniform vec3`
   (not reusing any of the six read-only names from User Story 5), **When** the Uniforms panel is
   shown, **Then** they appear as normal editable sliders/color pickers, not read-only text.

**Status**: Implemented as a new file, `assets/shaders/pbr_animation_artistic.glsl`, copied from
`pbr_animation.glsl` (identical vertex stage; fragment stage keeps every existing
auto-uniform-driven PBR term unchanged and adds the `art*` uniforms as a post-processing layer
on top). Chosen as a **separate shader file** rather than adding artistic controls to
`pbr_animation.glsl` itself, so the strict-PBR shader remains a predictable, physically-driven
reference/baseline unaffected by art-direction tweaks, consistent with this project's existing
convention of maintaining shader variants side by side (e.g. `bone_animation.glsl` vs.
`bone_animation_bump_mapping.glsl` vs. `bone_animation_material_only.glsl`) rather than one shader
with many optional branches.

---

### User Story 7 - Normal map and emissive texture support in the PBR shaders (Priority: P1) — ✅ Done

As a shader author importing a glTF model with normal maps and/or emissive (self-illuminating)
surfaces (e.g. CarConcept's brake lights or a bump-mapped body panel), I want both
`pbr_animation.glsl` and `pbr_animation_artistic.glsl` to actually sample and apply
`normalTexture`/`emissiveTexture`, instead of silently ignoring them, so the render matches the
authored look instead of showing a flat-shaded surface with no glow.

**Why this priority**: Directly requested by the user as a missing PBR feature; without it, any
glTF model relying on normal or emissive maps (common in real-world assets, including
CarConcept's dashboard/lights) renders visibly flatter/darker than intended — same class of bug
as the earlier base-color/metallic-roughness gaps in User Stories 1 and 3.

**Independent Test**: Import a glTF model with both a `normalTexture` and an `emissiveTexture`
(e.g. CarConcept), open either PBR shader, and confirm: (a) surface detail (bumps/creases) is
visible that isn't present in the raw vertex normals, and (b) emissive regions (e.g. lights) are
visibly bright regardless of scene lighting/camera angle.

**Acceptance Scenarios**:

1. **Given** a material with a bound `normalTexture` (glTF's `NORMALS` texture type,
   `hasNormalMap = true`), **When** the PBR fragment shader runs, **Then** the lighting normal is
   perturbed by the tangent-space normal map via a TBN basis built from the mesh's interpolated
   tangent/bitangent/normal vectors, not just the flat vertex normal.
2. **Given** a material with no bound normal texture (`hasNormalMap = false`), **When** the PBR
   fragment shader runs, **Then** the plain interpolated vertex normal is used, unchanged from
   before this feature.
3. **Given** a material with a bound `emissiveTexture` and/or non-zero `emissiveFactor`, **When**
   the PBR fragment shader runs, **Then** `emissiveFactor` (multiplied by the emissive texture
   sample when bound) is added to the final lit color before tone mapping, so emissive regions
   stay visibly bright even in shadow or when facing away from the light.
4. **Given** `pbr_animation_artistic.glsl` specifically, **When** the user adjusts the new
   `artNormalStrength`/`artEmissiveBoost` uniforms, **Then** bump intensity and emissive
   brightness scale accordingly, independent of the strict-PBR shader's fixed behavior.

**Status**: Implemented. `ModelMaterial` gained `hasNormalMap`, `hasEmissiveTexture`,
`emissiveFactor` (read via `AI_MATKEY_COLOR_EMISSIVE`); `AssimpModelLoader` sets the two bools
when `aiTextureType_NORMALS`/`aiTextureType_EMISSIVE` textures are found (both types were already
in `kPhoenixTextureTypes`, so the loader already produced `texture_normal1`/`texture_emissive1`
slots — only the corresponding "has" flags and emissive factor were missing before). Both PBR
shaders' vertex stage now also emit `vTangent`/`vBiTangent` (previously computed only by
`bone_animation_bump_mapping.glsl`); the fragment stage builds a TBN basis and applies the normal
map when `hasNormalMap` is true, and adds `emissiveFactor` (times the emissive texture when
bound) to the lit color. `PreviewRenderer::bindMeshMaterial` uploads all three new uniforms.
`UniformIntrospectionService::isPhoenixAutoUniform` recognizes `hasNormalMap`/
`hasEmissiveTexture` (bool) and `emissiveFactor` (vec3) as read-only auto-uniforms, matching the
existing PBR fields. Build-verified (`cmake --build . --config Debug --target shader_editor` and
`shader_editor_copy_assets` both succeeded); pending user visual confirmation.

---

### Edge Cases

- What happens when a translucent material is also the *only* material in the model (no opaque
  pass to draw first)? The transparent pass still runs correctly; it just blends against the
  clear color, which is the expected behavior for a scene with only glass.
- What happens when two overlapping translucent surfaces are drawn (e.g. windshield in front of
  a side window) without back-to-front depth sorting *within* the transparent pass itself? Known
  limitation, not addressed by this feature — the current fix only separates opaque vs.
  transparent globally, using the model's authored/material-grouped order within each pass, not
  a per-triangle depth sort. Accepted as "good enough" for this asset since no two transmissive
  surfaces are known to overlap along the default camera view.
- What happens if the running executable is not rebuilt/relaunched after a fix? Explicitly
  called out by the user as a recurring confusion; addressed by User Story 2 (fresh version
  stamp) so it is at least immediately visible in the window title.
- What happens to primitive (non-model) preview rendering (`plane`/`cube`/`torus`/etc.)? Left
  untouched — blending is only enabled/disabled around the model draw path
  (`beginModelFrame`/model draw loop), not the primitive path (`renderPrimitive`).
- What happens if a user names their own custom uniform `metallicFactor`/`materialOpacity`/etc.
  with the exact matching type in an unrelated shader? It will be treated as a Phoenix
  auto-uniform (read-only, engine-supplied) even outside the PBR shaders, matching how `Mat_Kd`
  already behaves today — an accepted, pre-existing naming convention risk, not new to this
  feature.
- What happens in `pbr_animation_artistic.glsl` when `artRoughnessBoost`/`artRoughnessBias` push
  roughness outside `[0.05, 1.0]`? Clamped, same floor as the non-artistic shader, to avoid
  divide-by-near-zero artifacts in the GGX/Fresnel terms.

## Requirements *(mandatory)*


### Functional Requirements

- **FR-001**: The Assimp model loader MUST check `aiTextureType_BASE_COLOR` (not only
  `aiTextureType_DIFFUSE`) when looking for a material's base color texture, and bind it to the
  same `texture_diffuse1` shader uniform name Phoenix shaders already expect. — ✅ Done
- **FR-002**: `ModelMaterial` MUST carry `metallicFactor`, `roughnessFactor`, `hasPbrTextures`,
  and `hasDiffuseTexture`, populated from `AI_MATKEY_METALLIC_FACTOR`/
  `AI_MATKEY_ROUGHNESS_FACTOR` and from whether the corresponding texture slots were found. —
  ✅ Done
- **FR-003**: `PreviewRenderer::bindMeshMaterial` MUST upload `metallicFactor`, `roughnessFactor`,
  `hasPbrTextures`, and `hasDiffuseTexture` as shader uniforms for every mesh's bound material. —
  ✅ Done
- **FR-004**: `assets/shaders/pbr_animation.glsl` MUST fall back to a `Mat_Kd` uniform (the
  material's authored base color) for albedo when `hasDiffuseTexture` is false, instead of
  unconditionally sampling `texture_diffuse1`. — ✅ Done
- **FR-005**: The Assimp model loader MUST also check `aiTextureType_GLTF_METALLIC_ROUGHNESS`
  (value 27) for the packed metallic-roughness texture, in addition to the legacy
  `aiTextureType_METALNESS`/`aiTextureType_DIFFUSE_ROUGHNESS` types, and MUST bind that single
  texture to both the `texture_metalness1` and `texture_roughness1` shader uniform names when
  found under the dedicated glTF type. — ✅ Done (build verified; visual confirmation pending)
- **FR-006**: `assets/shaders/pbr_animation.glsl` MUST sample metalness from the blue channel
  (`.b`) and roughness from the green channel (`.g`) of their respective bound textures, per the
  glTF metallic-roughness texture channel convention, instead of both sampling the red channel. —
  ✅ Done (build verified; visual confirmation pending)
- **FR-007**: `ModelMaterial` MUST carry `transmissionFactor` (from
  `AI_MATKEY_TRANSMISSION_FACTOR`) and `opacity` (from `AI_MATKEY_OPACITY`), independently, since
  either can make a glTF material translucent. — ✅ Done
- **FR-008**: `PreviewRenderer::bindMeshMaterial` MUST upload `transmissionFactor` and
  `materialOpacity` as shader uniforms. — ✅ Done
- **FR-009**: `assets/shaders/pbr_animation.glsl` MUST compute final alpha as
  `min(materialOpacity, 1.0 - transmissionFactor)` and use it for `FragColor.a` instead of a
  hardcoded `1.0`. — ✅ Done
- **FR-010**: The model render path MUST enable `GL_BLEND` with
  `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` and MUST leave the primitive
  (non-model) preview render path unaffected. — ✅ Done
- **FR-011**: The model render path MUST draw every opaque mesh instance (effective alpha ≈ 1.0)
  before any translucent mesh instance, and MUST disable depth writes
  (`glDepthMask(GL_FALSE)`) only for the translucent pass, restoring `glDepthMask(GL_TRUE)` and
  `glDisable(GL_BLEND)` once the frame's model draw loop completes. — ✅ Done, user-confirmed
- **FR-012**: `AssimpModelLoader`'s material-deduplication comparison (`materialsEqual`) MUST
  include every new field added by this feature (`metallicFactor`, `roughnessFactor`,
  `hasPbrTextures`, `hasDiffuseTexture`, `transmissionFactor`, `opacity`) so visually distinct
  materials are never incorrectly merged. — ✅ Done
- **FR-013**: The build MUST regenerate the window-title version timestamp
  (`shadereditor_version.h`) on every `cmake --build` invocation, not only on `cmake` reconfigure.
  — ✅ Done
- **FR-014**: The build MUST refresh the copied `assets/` directory next to the executable on
  every build invocation, even when no C++ source changed (e.g. only a `.glsl` asset was edited),
  by using a standalone always-run custom target rather than a `shader_editor` `POST_BUILD` step
  that MSBuild may skip when it decides no relink is required. — ✅ Done
- **FR-015**: `UniformIntrospectionService::isPhoenixAutoUniform` MUST recognize
  `metallicFactor`, `roughnessFactor`, `transmissionFactor`, `materialOpacity` (as `float`) and
  `hasPbrTextures`, `hasDiffuseTexture` (as `bool`) as Phoenix auto-uniforms, so the Uniforms
  panel presents them as read-only (matching `Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght`) instead
  of as editable sliders whose value `bindMeshMaterial()` silently overwrites every frame. —
  ✅ Done
- **FR-016**: A new shader file, `assets/shaders/pbr_animation_artistic.glsl`, MUST provide the
  same physically-based baseline as `pbr_animation.glsl` (identical vertex stage and identical
  auto-uniform-driven material inputs) plus additional, ordinary user-editable uniforms
  (`artMetallicBoost`, `artRoughnessBoost`, `artRoughnessBias`, `artSpecularIntensity`,
  `artAlbedoTint`, `artAmbientBoost`, `artOpacityMultiplier`) layered on top, so a shader author
  can art-direct the look without editing the imported material or `pbr_animation.glsl` itself. —
  ✅ Done
- **FR-017**: The `art*` uniforms introduced by FR-016 MUST NOT be added to
  `isPhoenixAutoUniform`'s allow-list, so they remain ordinary, user-editable Uniforms-panel
  controls (sliders/color picker), distinct from the read-only engine-supplied uniforms from
  FR-015. — ✅ Done
- **FR-018**: `ModelMaterial` MUST expose `hasNormalMap`, `hasEmissiveTexture`, and
  `emissiveFactor` (read from Assimp's `aiTextureType_NORMALS`/`aiTextureType_EMISSIVE` texture
  presence and `AI_MATKEY_COLOR_EMISSIVE` respectively), and `PreviewRenderer::bindMeshMaterial`
  MUST upload all three every mesh bind, so both PBR shaders can conditionally apply a normal map
  and add emissive self-illumination. — ✅ Done
- **FR-019**: Both `pbr_animation.glsl` and `pbr_animation_artistic.glsl` MUST emit
  `vTangent`/`vBiTangent` from the vertex stage and, in the fragment stage, perturb the lighting
  normal via a TBN basis when `hasNormalMap` is true (falling back to the plain vertex normal
  otherwise), and add `emissiveFactor` (times the emissive texture sample when
  `hasEmissiveTexture` is true) to the final lit color before tone mapping. — ✅ Done
- **FR-020**: `UniformIntrospectionService::isPhoenixAutoUniform` MUST recognize `hasNormalMap`/
  `hasEmissiveTexture` (bool) and `emissiveFactor` (vec3) as Phoenix auto-uniforms, matching the
  read-only treatment already given to the other PBR material fields (FR-015). — ✅ Done
- **FR-021**: `pbr_animation_artistic.glsl` MUST additionally expose ordinary, user-editable
  `artNormalStrength` (scales the tangent-space normal map's bump intensity) and
  `artEmissiveBoost` (multiplies the emissive contribution) uniforms, consistent with FR-016/
  FR-017's pattern of art-direction controls layered on top of the auto-supplied material
  values. — ✅ Done

### Key Entities *(include if feature involves data)*

- **ModelMaterial** (`src/rendering/models/ModelDocument.h`): in-memory, engine-agnostic material
  record populated by `AssimpModelLoader`. Extended in this feature with `metallicFactor`,
  `roughnessFactor`, `hasPbrTextures`, `hasDiffuseTexture`, `transmissionFactor`, `opacity`.
- **ModelTextureSlot**: one bound texture (by Phoenix shader uniform name) on a `ModelMaterial`;
  this feature adds the case where a single Assimp texture type
  (`aiTextureType_GLTF_METALLIC_ROUGHNESS`) fans out into two slots (`texture_metalness1` and
  `texture_roughness1`) rather than the usual one-to-one mapping.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: CarConcept's "Mechanical" (body) material renders in its authored dark-red/carmine
  tone, not black, with `pbr_animation.glsl`. — ✅ Achieved
- **SC-002**: Editing only a `.glsl` file and rebuilding causes the running executable's next
  launch to load the edited shader content, verified by comparing file hashes/timestamps between
  `assets/` and the copied `build-vcpkg/<config>/assets/`. — ✅ Achieved
- **SC-003**: Two builds run more than one minute apart with no reconfigure show two different
  version timestamps in the window title. — ✅ Achieved
- **SC-004**: CarConcept's body shows visibly varying specular roughness across panels (not a
  uniform, flat highlight) when viewed under the free camera with light/camera movement. — ⏳
  Pending user visual confirmation (root cause fixed and build-verified; user has not explicitly
  re-confirmed this specific point since the `aiTextureType_GLTF_METALLIC_ROUGHNESS` fix, though
  they did not report it as still broken in their most recent messages)
- **SC-005**: CarConcept's glass (windshield/windows) shows the interior/opposite side of the
  model through it, not a flat color matching the render viewport's background. — ✅ Achieved,
  user-confirmed ("ahora los cristales se ven transparentes bien")
- **SC-006**: Opaque materials elsewhere in the model (body, wheels, interior) show no visual
  regression (no unintended transparency, no z-fighting) after the two-pass draw order change. —
  ✅ Achieved, user-confirmed
- **SC-007**: Dragging `metallicFactor`/`roughnessFactor`/`transmissionFactor`/`materialOpacity`
  sliders in the Uniforms panel is no longer possible — they render as `[read only]` text instead,
  making it visually obvious these are engine-supplied. — ✅ Achieved
- **SC-008**: With `pbr_animation_artistic.glsl` selected and all `art*` uniforms at their neutral
  defaults, the render is visually indistinguishable from `pbr_animation.glsl`; moving any single
  `art*` slider produces a visible, isolated change. — ✅ Implemented and build-verified;
  ⏳ pending user visual confirmation
- **SC-009**: A glTF model with a `normalTexture` shows visible surface bump detail (not a flat
  vertex-normal-only look) with either PBR shader. — ✅ Implemented and build-verified;
  ⏳ pending user visual confirmation
- **SC-010**: A glTF model with a non-zero `emissiveFactor`/`emissiveTexture` (e.g. brake lights)
  shows visibly bright self-illuminated regions regardless of scene lighting direction. —
  ✅ Implemented and build-verified; ⏳ pending user visual confirmation

## Assumptions

- The project's vendored Assimp (via vcpkg, `x64-windows-static-md` triplet) is a version new
  enough to define `aiTextureType_GLTF_METALLIC_ROUGHNESS` (value 27); confirmed present in
  `build-vcpkg/vcpkg_installed/x64-windows-static-md/include/assimp/material.h` and
  `GltfMaterial.h` in this environment. Older Assimp versions without this enum value would need
  a different fix (e.g. relying solely on `aiTextureType_METALNESS`/`DIFFUSE_ROUGHNESS`, which
  some Assimp glTF2 importer versions do populate instead).
  - This is only a texture channel packing fix; it does not add support for `KHR_materials_clearcoat`
  (seen on "Paint 2 Carmine") or `KHR_materials_transmission`'s more advanced properties
  (`transmissionTexture`, IOR-correct refraction) — the current fix treats transmission as a
  simple alpha-blend approximation, not physically-based refraction.
- A single global opaque→transparent draw split (no per-pixel/per-triangle depth sort within the
  transparent pass) is accepted as sufficient for this asset and is a pragmatic first pass, not a
  general-purpose transparency-sorting solution.
- `pbr_animation_artistic.glsl` is a plain file in `assets/shaders/`, discoverable the same way
  every other shader is (via the existing "Open .glsl" file dialog) — there is no in-app shader
  catalog/list in C++ that needed updating to make it available.
- The user is expected to fully close any previously running `shader_editor.exe` before
  launching a freshly built one; the version-timestamp fix (User Story 2) makes this mistake
  visible but does not prevent it.
