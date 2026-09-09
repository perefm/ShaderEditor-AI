# Implementation Plan: Built-In Primitive UV Mapping and Texture-State Isolation Fixes

**Branch**: `009-primitive-uv-texture-fixes` | **Date**: 2026-09-09 | **Spec**: [spec.md](./spec.md)

## Summary

A bug-fixing pass (not new UI/UX) covering two independent problem areas surfaced while manually
texturing built-in primitives and switching between primitives/imported Assimp models:

1. **UV mapping**: the built-in primitive library (`plane`, `cube`, `sphere`, `cylinder`, `torus`)
   generated every shape's texture coordinates from one shared spherical-projection formula
   (`atan2`/`asin` of vertex position), which only makes sense for the sphere. This produced a
   heavily distorted "wrapped" look on the cube in particular. Rewritten so each shape has its own
   conventional UV generator, plus a single project-wide vertical flip applied once at the end to
   match `stb_image`'s flip-on-load decoding convention (root cause of the separately reported
   "texture is upside-down" bug, which — after a first, per-shape patch attempt on just the plane
   and cube — turned out to affect every shape equally and was better fixed in one place).
2. **Texture-uniform state isolation**: `sampler2D` uniform values (manually assigned via "Load
   Image") were being persisted by name+type across render-target switches exactly like ordinary
   `float`/`vec3` uniforms, and the renderer silently skipped re-binding a texture unit whenever a
   uniform's value was empty. Combined, these two facts meant a texture could stay bound in a GPU
   texture unit — and visibly rendered — for a render target it does not belong to (a model
   showing a manually loaded texture, or a primitive showing a model's imported texture). Fixed by
   (a) explicitly unbinding the texture unit when a sampler's value is empty, and (b) giving
   "built-in primitives" and "the active model" independent, explicitly captured/restored texture
   uniform sets in `WorkspaceController`, instead of one flat, always-preserved-by-name set.

## Technical Context

**Language/Version**: C++20
**Primary Dependencies**: OpenGL 4.6 core, GLFW, Dear ImGui docking, GLM, glad, Assimp, stb_image
**Storage**: No new persistent storage
**Testing**: Existing CTest target `shader_editor_tests`; validated primarily by manual build +
visual inspection (texture orientation/mapping is fundamentally a GL-context/visual concern — see
"Testing Gap" below), plus a full run of the existing automated suite after every change to guard
against regressions.
**Target Platform**: Windows desktop (MSVC, CMake + vcpkg)
**Project Type**: Single desktop application (`src/`, `tests/`)
**Performance Goals**: No new per-frame allocations; the primitive UV rewrite is generated once at
startup (`PrimitiveLibrary`'s constructor), not per frame; the texture capture/restore maps
touched are tiny (one entry per `sampler2D` uniform the active shader declares).
**Constraints**: Must not regress existing PBR/transparency/animation/camera model-rendering
behavior from spec 007, nor the primitive draw path's shared program/uniform plumbing.

## Root Causes Found (for future reference)

| Symptom | Root Cause |
|---|---|
| Cube face shows a distorted sliver of the texture, not the full image | UV generation used one shared spherical-projection formula (`0.5 + atan2(z,x)/2π`, `0.5 + asin(clamp(y,-1,1))/π`) for every primitive, including the cube, instead of a per-face "dice" mapping |
| After importing a model, switching to Sphere/Torus still showed the model's texture | `applyUniforms()` (primitive draw path) `continue`d without touching GL state when a `sampler2D` uniform's value was empty, leaving whatever texture a *previous* draw (the model's `bindMeshMaterial()`) had bound in that same texture unit still bound and visibly sampled |
| After manually loading a texture, then importing a model, the model showed the manual texture instead of its own | `UniformState::setDefinitions()` preserves `currentValue` for any uniform whose name+kind matches the previous shader's — including `sampler2D` uniforms — so a manually assigned texture path silently carried over to the model's own same-named sampler uniform (e.g. `texture_diffuse1`), and nothing cleared it before `bindMeshMaterial()` had a chance to overwrite it (which it only does for slots the model's material actually populates) |
| Switching between primitives (e.g. plane → sphere) lost the manually assigned texture | The first fix attempt unconditionally cleared every `sampler2D` uniform on every `selectPrimitive()`/`openModel()`/`selectModel()` call, which also wiped it when the user was just switching between primitives, not entering/leaving model mode |
| Plane/cube textures appeared upside-down (first report) | UV mapping for `plane`/`cube` was authored with the conventional "v=0 at top" convention, but `stbi_set_flip_vertically_on_load(1)` (used when uploading any texture) stores the image's bottom row as GL row 0 — a mismatch that a first, per-shape patch (manually flipping only `plane`'s and `cube`'s UVs) only partially fixed |
| Torus/sphere/cube *still* upside-down after the per-shape patch (second report) | The flip mismatch above is universal, not specific to the plane/cube — every shape's UV generator shares the same "v=0 at top" convention and needs the same flip; patching individual shapes both under- and over-corrected depending on which shapes had already been touched |

## Files Changed

- `src/rendering/geometry/PreviewPrimitive.h` — doc-comment on `texcoords` updated to describe the
  per-shape mapping convention (no structural change).
- `src/rendering/geometry/BuiltInPrimitives.cpp` — full rewrite of UV generation:
  - New `appendQuad(PreviewPrimitive&, ...)` / `appendTriangleWithUv(...)` helpers that take
    explicit per-vertex UVs at mesh-construction time, replacing the old position-only
    `appendQuad(vector<vec3>&, ...)` + a separate `generateUvMap()` post-pass.
  - `makePlane()`: simple planar `[0,1]x[0,1]` mapping.
  - `makeCube()`: "dice" mapping — each of the 6 faces gets its own full `[0,1]x[0,1]` UV
    rectangle.
  - `makeSphere()`: conventional equirectangular (u = longitude/slice angle, v = latitude/stack
    angle) mapping.
  - `makeCylinder()`: side wraps circumferentially (u) and top-to-bottom (v); each cap gets its
    own disc mapping centered at `(0.5, 0.5)` instead of reusing the side's cylindrical UVs.
  - `makeTorus()`: toroidal mapping (u = major/ring angle, v = minor/tube angle).
  - `makeBuiltInPrimitives()`: after generating all shapes, applies one project-wide
    `uv.y = 1.0F - uv.y` flip over every primitive's texcoords, to match `stb_image`'s
    flip-on-load decoding convention in a single place rather than baking ad-hoc flips into
    individual shape formulas.
- `src/rendering/opengl/PreviewRenderer.cpp` — `applyUniforms()`: when a `sampler2D` uniform's
  current value is empty, now explicitly issues `glActiveTexture` + `glBindTexture(GL_TEXTURE_2D,
  0)` + `glUniform1i` for that texture unit (previously `continue`d with no GL calls at all),
  so an empty sampler value can never keep sampling whatever texture a different render target
  left bound in that unit.
- `src/app/workspace/UniformState.h` / `.cpp` — new methods:
  - `clearTextureValues()` — resets every `sampler2D` uniform's current value to empty.
  - `captureTextureValues() const` — returns a `name -> path` map of every `sampler2D` uniform's
    current value.
  - `restoreTextureValues(const unordered_map<string,string>&)` — applies a previously captured
    map back onto matching-by-name `sampler2D` uniforms, leaving uncovered ones untouched.
- `src/app/workspace/WorkspaceController.h` / `.cpp` —
  - New member `primitiveTextureValues_`: the texture assignments shared by all built-in
    primitives, snapshotted whenever the user leaves primitive mode for model mode.
  - `selectPrimitive()`: only clears+restores texture uniforms when the *previous* render target
    was a model (i.e. an actual mode change); switching between primitives leaves texture
    uniforms completely untouched, preserving user intent from spec's User Story 3.
  - `openModel()` / `selectModel()`: before switching to `RenderTargetKind::Model`, capture the
    current texture values into `primitiveTextureValues_` if the previous target was a primitive;
    then clear all texture uniforms so the model always starts showing only its own imported
    textures.

## Testing Gap / Follow-Up

No new automated unit test was added for:
- the per-shape UV mapping formulas (cube's dice mapping, sphere's equirectangular mapping, etc.),
- the vertical-flip-on-load correctness,
- the empty-sampler texture-unit unbind behavior in `PreviewRenderer::applyUniforms`,
- the primitive/model independent texture-set capture-and-restore behavior in
  `WorkspaceController`,

because these are fundamentally either (a) geometry/UV data that is only meaningfully verified by
rendering a known texture and visually inspecting orientation/distortion, or (b) live-GL-context
texture-binding state that the project's current headless unit tests cannot exercise. If this
project later adds pixel-comparison or headless-GL test infrastructure, good candidates to
formalize would be:
- a unit test asserting each `PreviewPrimitive`'s texcoords stay within `[0,1]` and that the cube
  produces exactly 6 distinct full-rectangle UV sets (one per face),
- a unit test asserting `UniformState::captureTextureValues()` /
  `restoreTextureValues()` round-trip correctly and that `clearTextureValues()` only affects
  `sampler2D`-typed uniforms,
- a `WorkspaceController`-level test asserting a manually-assigned texture value survives a
  primitive→primitive switch but is cleared by a primitive→model switch and restored by the
  matching model→primitive switch back.

## Verification Performed This Session

- `cmake --build build-vcpkg --config Debug` succeeded after every round of source changes (no
  compile errors introduced), rebuilding `shader_editor_core`, `shader_editor`, and
  `shader_editor_tests`.
- `shader_editor_tests.exe --success` run after every change; full suite passed (exit code 0)
  each time, confirming no regression to existing render-session, uniform-introspection,
  Assimp-loader, skeletal-animator, or model-info coverage.
- Manual reasoning verification (no live-GL harness available in this environment) of the UV
  formulas against the conventional mapping for each shape, and of the single end-of-generation
  flip against `stbi_set_flip_vertically_on_load(1)`'s documented effect.
