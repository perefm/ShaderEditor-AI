# Implementation Plan: glTF PBR Material Correctness (Base Color, Metallic-Roughness, Transmission/Transparency) and Build Freshness

**Branch**: `007-gltf-pbr-transparency-fixes` | **Date**: 2026-09-09 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/007-gltf-pbr-transparency-fixes/spec.md`

**Reference Asset**: [KhronosGroup/glTF-Sample-Assets - CarConcept](https://github.com/KhronosGroup/glTF-Sample-Assets/tree/main/Models/CarConcept)

## Summary

A bug-fixing pass (not new UI/UX) making imported glTF PBR materials — specifically the
Khronos "CarConcept" sample — render correctly with `assets/shaders/pbr_animation.glsl`:

1. Base color: bind glTF's `baseColorTexture` (Assimp's `aiTextureType_BASE_COLOR`, distinct
   from `aiTextureType_DIFFUSE`) and fall back to `baseColorFactor` when no texture exists.
2. Metallic-roughness workflow: read `metallicFactor`/`roughnessFactor` scalars, and — the root
   cause of "roughness still looks completely flat" — recognize that this project's Assimp
   version exposes the packed `metallicRoughnessTexture` only under the dedicated
   `aiTextureType_GLTF_METALLIC_ROUGHNESS` type (not the legacy `METALNESS`/`DIFFUSE_ROUGHNESS`
   types originally checked), and sample it with the glTF-correct channel packing
   (metalness = `.b`, roughness = `.g`).
3. Transparency: read `KHR_materials_transmission`'s `transmissionFactor` and glTF alpha/opacity,
   compute a combined alpha in the shader, enable GL blending for the model draw path only, and
   — the root cause of "glass renders as opaque background color" — split the draw order into
   an opaque pass (depth writes on) followed by a transparent pass (depth writes off), so glass
   blends against the real, already-drawn scene instead of an empty framebuffer.
4. Build freshness: make the window-title version timestamp and the copied runtime `assets/`
   directory regenerate on every `cmake --build`, not only on reconfigure / when MSBuild decides
   a target needs relinking — this was independently causing "my fix isn't showing up" reports
   twice in this session.

## Technical Context

**Language/Version**: C++20
**Primary Dependencies**: OpenGL 4.6 core, GLFW, Dear ImGui docking, GLM, glad, Assimp (vcpkg
`x64-windows-static-md` triplet; version confirmed to define `aiTextureType_GLTF_METALLIC_ROUGHNESS`
in `assimp/material.h`/`GltfMaterial.h`), stb_image
**Storage**: No new persistent storage
**Testing**: Existing CTest target `shader_editor_tests`; this feature was validated primarily by
manual build + visual inspection against the CarConcept reference renders (no new automated test
was added for shader channel sampling or blend state, since these require a live GL context/visual
comparison rather than being unit-testable in isolation — see "Testing gap" below)
**Target Platform**: Windows desktop (MSVC, CMake + vcpkg)
**Project Type**: Single desktop application (`src/`, `tests/`)
**Performance Goals**: No new per-frame allocations; the opaque/transparent split reuses the
existing `materialSortedMeshOrder()` cache (computed once per model load, not per frame)
**Constraints**: Must not affect the primitive (non-model) preview render path
(`renderPrimitive`); must not regress existing bone/node animation or camera behavior from spec
006

## Root Causes Found (for future reference)

| Symptom | Root Cause |
|---|---|
| Car body rendered pure black | `aiTextureType_DIFFUSE` was checked but glTF's `baseColorTexture` is `aiTextureType_BASE_COLOR`; combined with the "Mechanical" material having no base color texture at all (only a dark `baseColorFactor`), the shader sampled an unbound texture unit |
| Fix "not showing up" after rebuild (round 1) | Old `shader_editor.exe` instance was still running; window title showed a stale timestamp frozen at the last CMake *configure* (not build) |
| `string(TIMESTAMP ...)` frozen | CMake's `string(TIMESTAMP)` only re-evaluates on reconfigure, not on every `cmake --build` |
| Body color correct but roughness flat / glass not transparent | Two independent uniform-upload/shader gaps: no metallic-roughness texture sampling wired up at all, and `FragColor.a` hardcoded to `1.0` with no `GL_BLEND` enabled anywhere |
| Roughness *still* flat after wiring up sampling + channel swizzle | The loader was checking `aiTextureType_METALNESS`/`aiTextureType_DIFFUSE_ROUGHNESS`, but this Assimp version puts glTF's packed texture under the dedicated `aiTextureType_GLTF_METALLIC_ROUGHNESS` (27) instead — so `hasPbrTextures` was always false and the flat scalar factors were always used |
| Glass rendered as opaque background color, not see-through | Meshes were drawn in material-index order with `GL_BLEND` on for the whole model pass but no depth-write/pass separation — a transparent mesh drawn early blended against a still-mostly-empty framebuffer (just the clear color) instead of the opaque scene, and its depth write then occluded opaque geometry drawn afterward |
| Fix "not showing up" after rebuild (round 2, this session) | `assets/` copy was a `POST_BUILD` step on the `shader_editor` *executable* target; MSBuild can skip `POST_BUILD` commands when it determines the target itself needs no relink (e.g. only a `.glsl` asset changed, no C++ recompiled), so the copied shader next to the exe silently went stale |

## Files Changed

- `src/rendering/models/ModelDocument.h` — `ModelMaterial` gains `metallicFactor`,
  `roughnessFactor`, `hasPbrTextures`, `hasDiffuseTexture`, `transmissionFactor`, `opacity`.
- `src/rendering/models/AssimpModelLoader.cpp` —
  - `kPhoenixTextureTypes` extended with `aiTextureType_BASE_COLOR` and
    `aiTextureType_GLTF_METALLIC_ROUGHNESS`.
  - Reads `AI_MATKEY_METALLIC_FACTOR`, `AI_MATKEY_ROUGHNESS_FACTOR`,
    `AI_MATKEY_TRANSMISSION_FACTOR`, `AI_MATKEY_OPACITY`.
  - Special-cases `aiTextureType_GLTF_METALLIC_ROUGHNESS`: binds the single packed texture to
    both `texture_metalness1` and `texture_roughness1` shader uniform names.
  - `materialsEqual()` updated to compare every new field (dedup correctness).
- `src/rendering/opengl/PreviewRenderer.h` — new `isMaterialTransparent(const ModelMaterial&)`
  static helper declaration; `materialSortedMeshOrder()` doc-comment updated to describe the
  opaque-before-transparent guarantee.
- `src/rendering/opengl/PreviewRenderer.cpp` —
  - `bindMeshMaterial()` uploads `metallicFactor`, `roughnessFactor`, `hasPbrTextures`,
    `hasDiffuseTexture`, `transmissionFactor`, `materialOpacity`.
  - `beginModelFrame()` enables `GL_BLEND` / `glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)`.
  - Model draw loop in `renderFrame()`: `glDepthMask(GL_FALSE)` the first time a transparent
    material is encountered in the (now opaque-first) sorted draw order; restores
    `glDepthMask(GL_TRUE)` / `glDisable(GL_BLEND)` after the loop. Primitive draw path
    untouched.
  - `materialSortedMeshOrder()`: stable-sort comparator now sorts all opaque instances before
    all transparent ones (using the new `isMaterialTransparent` helper), falling back to the
    original material-index ordering within each group.
  - `isMaterialTransparent()`: mirrors the shader's alpha formula,
    `min(opacity, 1 - transmissionFactor) < 0.999`.
- `assets/shaders/pbr_animation.glsl` —
  - `hasDiffuseTexture`/`Mat_Kd` uniforms; albedo falls back to `Mat_Kd` when no diffuse texture.
  - `metallic`/`roughness` now conditioned on `hasPbrTextures`, sampling `.b`/`.g` respectively
    (previously both incorrectly sampled `.r`).
  - `transmissionFactor`/`materialOpacity` uniforms; `FragColor.a` now
    `min(materialOpacity, 1.0 - transmissionFactor)` instead of a hardcoded `1.0`.
- `CMakeLists.txt` —
  - Removed frozen `string(TIMESTAMP ...)`-driven `target_compile_definitions`.
  - Added `shader_editor_version_header` custom target (`ALL`, always reruns) generating
    `generated/shadereditor_version.h` fresh every build.
  - Added `shader_editor_copy_assets` custom target (`ALL`, always reruns, depends on
    `shader_editor`) replacing the old `POST_BUILD` asset copy step that MSBuild could skip.
- `cmake/GenerateVersionHeader.cmake` (new) — script invoked by
  `shader_editor_version_header`; only rewrites the header file if the timestamp actually
  changed, to avoid unnecessary downstream relinks.
- `src/app/application/Application.cpp` — `#include <shadereditor_version.h>` instead of relying
  on a compile-time `-D` macro.
- `src/rendering/shaders/UniformIntrospectionService.cpp` — `isPhoenixAutoUniform` extended to
  recognize `metallicFactor`/`roughnessFactor`/`transmissionFactor`/`materialOpacity` (float) and
  `hasPbrTextures`/`hasDiffuseTexture` (bool) as read-only, engine-supplied uniforms, matching the
  existing `Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght` entries.
- `assets/shaders/pbr_animation_artistic.glsl` (new) — art-directable PBR shader variant; same
  vertex stage and auto-uniform-driven inputs as `pbr_animation.glsl`, with `artMetallicBoost`,
  `artRoughnessBoost`, `artRoughnessBias`, `artSpecularIntensity`, `artAlbedoTint`,
  `artAmbientBoost`, `artOpacityMultiplier` layered on top as ordinary editable uniforms.
- `src/rendering/models/ModelDocument.h` — `ModelMaterial` gained `hasNormalMap`,
  `hasEmissiveTexture`, `emissiveFactor`.
- `src/rendering/models/AssimpModelLoader.cpp` — reads `AI_MATKEY_COLOR_EMISSIVE`; sets
  `hasNormalMap`/`hasEmissiveTexture` from `aiTextureType_NORMALS`/`aiTextureType_EMISSIVE`
  texture presence (both types were already loaded into `texture_normal1`/`texture_emissive1`
  slots, just without a corresponding "has" flag or emissive factor); `materialsEqual` updated.
- `src/rendering/opengl/PreviewRenderer.cpp` — `bindMeshMaterial` uploads `hasNormalMap`,
  `hasEmissiveTexture`, `emissiveFactor`.
- Both `assets/shaders/pbr_animation.glsl` and `pbr_animation_artistic.glsl` — vertex stage now
  also emits `vTangent`/`vBiTangent`; fragment stage applies a TBN-based normal map when
  `hasNormalMap` and adds `emissiveFactor` (times the emissive texture when bound) to the lit
  color. The artistic variant additionally exposes `artNormalStrength`/`artEmissiveBoost`.
- `src/rendering/shaders/UniformIntrospectionService.cpp` — `isPhoenixAutoUniform` extended
  again to recognize `hasNormalMap`/`hasEmissiveTexture` (bool) and `emissiveFactor` (vec3).

## Testing Gap / Follow-Up

No new automated unit test was added for:
- the metallic-roughness channel swizzle or the `aiTextureType_GLTF_METALLIC_ROUGHNESS` mapping,
- the shader's alpha formula or the opaque/transparent draw split,
- the CMake asset-copy/version-header "always reruns" behavior,

because these are fundamentally either (a) GLSL shader logic requiring a live GL context and
visual/pixel comparison to verify meaningfully, or (b) build-system behavior best verified by
actually invoking `cmake --build` twice and diffing output, as was done manually in this
session. If this project later adds pixel-comparison or headless-GL test infrastructure, the
following would be good candidates to formalize:
- unit test asserting `AssimpModelLoader` produces two texture slots
  (`texture_metalness1`/`texture_roughness1`) from one `aiTextureType_GLTF_METALLIC_ROUGHNESS`
  texture entry,
- unit test asserting `PreviewRenderer::isMaterialTransparent` classifies a
  `transmissionFactor = 1.0` material as transparent and a default material as opaque,
- a CMake/CTest smoke check that `shader_editor_copy_assets` and
  `shader_editor_version_header` targets exist and are members of `ALL`.

## Verification Performed This Session

- `cmake --build . --config Debug --target shader_editor` succeeded after every round of source
  changes (no compile errors introduced).
- `cmake .` reconfigure + two builds ~65s apart confirmed the version timestamp advanced
  (`10:45` → `10:48`, later `10:58` → `11:10` → `11:21` across the session).
- Confirmed via file hash comparison that `assets/shaders/pbr_animation.glsl` and its copy in
  `build-vcpkg/Debug/assets/shaders/pbr_animation.glsl` matched after a build using the new
  `shader_editor_copy_assets` target.
- Confirmed in `build-vcpkg/vcpkg_installed/x64-windows-static-md/include/assimp/material.h` and
  `GltfMaterial.h` that `aiTextureType_GLTF_METALLIC_ROUGHNESS = 27` exists and is the type
  `AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE` resolves to in this project's
  vendored Assimp.
- Visual confirmation from the user: body color (SC-001), build freshness (SC-002/SC-003), glass
  transparency (SC-005/SC-006), and read-only PBR uniforms (SC-007) all confirmed working. Body
  roughness variation (SC-004) has not been explicitly re-confirmed since the
  `aiTextureType_GLTF_METALLIC_ROUGHNESS` root-cause fix, though the user has not reported it as
  still broken either. The new artistic shader (SC-008) builds cleanly and copies to the runtime
  `assets/` folder correctly but has not yet been opened/exercised by the user.
- Added `aiTextureType_GLTF_METALLIC_ROUGHNESS` handling, the read-only Uniforms-panel fix, and
  `pbr_animation_artistic.glsl`; rebuilt with `cmake --build . --config Debug --target
  shader_editor` and `shader_editor_copy_assets` (window-title timestamp `2026.09.09-11:29`),
  both succeeding with no compile errors.

## Next Steps (for whoever continues this feature)

1. Ask the user to fully close any running `shader_editor.exe` and relaunch the build with
   window-title timestamp `2026.09.09-11:29` or later.
2. Explicitly re-confirm SC-004 (roughness variation visible on the car body) — this was the one
   remaining unconfirmed item from the strict-PBR shader work; if still flat, double-check that
   the "Mechanical" material's ORM texture actually resolves via
   `aiTextureType_GLTF_METALLIC_ROUGHNESS` (not silently zero textures found) and that
   `Mechanical_ORM.png`'s path resolves correctly relative to the model directory.
3. Have the user try `assets/shaders/pbr_animation_artistic.glsl` against CarConcept (or any
   other imported PBR model) and confirm SC-008: identical look at neutral `art*` defaults, and a
   visible, isolated change when each `art*` slider is moved.
4. Once SC-004 and SC-008 are confirmed, mark every remaining ⏳ item in `spec.md` as ✅ Done and
   move this spec folder to `specs/archive/` per this project's convention (see
   `specs/archive/006-keyframe-animation-cameras-model-info`).
