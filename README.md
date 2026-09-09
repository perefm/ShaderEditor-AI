# ShaderEditor

ShaderEditor is a desktop OpenGL 4.6 shader workspace for editing Phoenix-style
single-file GLSL shaders, previewing them on built-in 3D primitives, and arranging the
tool panels in a dockable Dear ImGui layout.

## Features

- Load, edit, and save Phoenix `.glsl` files containing `#type vertex` and `#type fragment`
- `Save shader` and `Update Shader` actions for the active shader document
- `Save As...` support for writing the current shader pair to a new path
- Live preview on plane, cube, torus, sphere, and cylinder
- Assimp model import from the File menu or the Render panel (`.glb`, `.gltf`, `.fbx`,
  `.obj`, and `.dae`)
- Imported model materials and textures, including textures embedded in `.glb` files
- Skeletal animation playback with an animation selector in the Render panel
- Model-size-aware orbit, pan, zoom, framing, and clipping
- Dockable editor, render, uniforms, diagnostics, configuration, and shader error panels
- Configurable shader-editor text size and VSync in the `Config` panel
- Render-panel FPS readout and configurable preview background color
- `Ctrl+Enter` and button-driven shader recompilation
- Editable runtime uniforms including `float`, `int`, `bool`, `vec2`, `vec3`,
  `vec4`, `mat2`, `mat3`, `mat4`, and `sampler2D`
- Mouse-driven preview navigation
  - Left drag: orbit the free camera around the scene
  - Right drag: pan the free camera
  - `Reset View`: restore the default framing
- GLM-based math pipeline for preview transforms and uniform upload
- Phoenix-compatible engine-provided shader uniforms and vertex attributes
- Bundled shader catalog covering plain examples, a single "mega" material
  shader for any Assimp import (bones + PBR/Blinn-Phong + every material map),
  stylized variants (toon/cel shading, rim lighting), and the legacy
  per-feature shaders kept as reference (see "Bundled shaders" below and
  [assets/shaders/README.md](assets/shaders/README.md))
- Built-in primitives (plane/cube/torus/sphere/cylinder) and imported Assimp
  models share the same 7-attribute vertex layout, so any bundled shader can
  render either a primitive or a loaded model interchangeably

### Engine-provided uniforms

The preview engine supplies these Phoenix-compatible values automatically on
every rendered frame or mesh draw. They are never discovered by uniform
introspection, so they never appear in the `Uniforms` panel and cannot be
edited there — declaring them in a shader is enough to receive them:

- `uniform mat4 view`: view matrix of the active camera (free camera or the
  selected model camera), exactly as Phoenix supplies it.
- `uniform mat4 projection`: projection matrix of the active camera.
- `uniform mat4 MVP`: model-view-projection matrix for the preview. It is always
  computed as `projection * view * model`, so the three matrices can never
  disagree.
- `uniform mat4 model`: model matrix of the mesh being drawn, as authored in the
  model file, times its animated scene-node transform for node-keyframed
  objects. Orbit and pan move the free camera rather than the model, so this
  never contains preview navigation. Useful for transforming normals and
  tangents into world space.
- `uniform vec3 uCameraPos`: world-space position of the **active** camera. When
  a model camera is selected it follows that camera, including its keyframe
  animation, and is re-evaluated every frame.
- `uniform float t`: elapsed playback time in seconds.
- `uniform float tend`: configured playback section duration in seconds; it
  is always greater than `1.0` and reaching it resets `t` to `0.0`.
- `uniform float beat`: normalized current beat phase in `[0, 1)`, derived from
  elapsed time and BPM; it resets to `0` at each beat boundary.
- `uniform float vpWidth`: current Render preview viewport width in pixels.
- `uniform float vpHeight`: current Render preview viewport height in pixels.
- `uniform float aspectRatio`: current Render preview viewport width divided by
  height.
- `uniform vec3 Mat_Ka`, `Mat_Kd`, `Mat_Ks`: active mesh ambient, diffuse, and
  specular material colors.
- `uniform float Mat_KsStrenght`: active mesh specular strength.
- `uniform mat4 gBones[100]`: active skeletal animation bone transforms.
- `texture_*` sampler uniforms such as `texture_diffuse1`,
  `texture_specular1`, `texture_normal1`, `texture_emissive1`,
  `texture_metalness1`, `texture_roughness1`, and `texture_height1`: active
  mesh texture slots, loaded from external files or embedded model images.
- `uniform float metallicFactor`, `roughnessFactor`, `transmissionFactor`,
  `materialOpacity`: active mesh glTF metallic-roughness/transmission factors.
- `uniform bool hasPbrTextures`, `hasDiffuseTexture`, `hasNormalMap`,
  `hasEmissiveTexture`, `hasSpecularMap`, `hasHeightMap`, `hasPbrWorkflow`:
  whether the active mesh has a dedicated texture/workflow for that channel,
  so a shader can fall back to the scalar factors/colors above when it does
  not. All of these flags are engine-supplied and read-only in the `Uniforms`
  panel, exactly like the textures/factors they gate.
- `uniform vec3 emissiveFactor`: active mesh glTF emissive color.

The bundled `assets/models/NormalTangentTest/NormalTangentTest.glb` is a
royalty-free Khronos CC0 sample with embedded normal maps; use it with
`mega_material.glsl` (or `legacy/bump_mapping.glsl`) to validate tangent-space
bump mapping. See "Bundled shaders" below for a description of every shader
shipped with the project.

Imported model vertex attributes **and** built-in primitives now share one
vertex layout: `aPos` (0), `aNormal` (1), `aTexCoords` (2), `aTangent` (3),
`aBiTangent` (4), `aBoneID` (5), and `aBoneWeight` (6). Primitives compute flat
per-face normals/tangents at generation time and upload zeroed bone
ids/weights (resolving to an identity skin transform), so **any** bundled
shader — whether written for a primitive or for an imported model — renders
correctly against either kind of render target.

Declare and use these names in shader stages that need them. Other uniforms
declared by the shader are discovered after a successful compile and remain
editable from the `Uniforms` panel.

### Bundled shaders

All bundled shaders live in `assets/shaders/`. Each is a single `.glsl` file
with a `#type vertex` and `#type fragment` section. See
[assets/shaders/README.md](assets/shaders/README.md) for the full catalog and
[specs/archive/008-mega-shader-pbr-lighting/contracts/shader-catalog.md](specs/archive/008-mega-shader-pbr-lighting/contracts/shader-catalog.md)
for the original design contract. Since the primitive and imported-model
vertex layouts are unified (see above), every shader below can be applied to
a built-in primitive or an imported model.

**Category 1 — learning / template** (no Assimp material data or bones):

- `basic.glsl` — flat-shaded solid color from a single `uniform vec3 color`;
  the minimal starting point for a new shader.
- `uniforms.glsl` — demonstrates editable `float`/`vec4` uniforms
  (`intensity`, `tint`) with no lighting.
- `diagnostics.glsl` — outputs solid white; useful for isolating whether a
  problem is in geometry/transform setup or in shading.
- `textured.glsl` — samples a single `sampler2D imageTexture` and outputs it
  unlit.
- `pixel_lighting.glsl` — per-pixel ambient + diffuse + specular lighting
  using a face normal derived from screen-space derivatives (`dFdx`/`dFdy`).

**Category 2 — realistic Assimp materials**:

- `mega_material.glsl` — **recommended default shader** for any Assimp-imported
  model. Supports skeletal animation (with a static-model fallback), a PBR
  metallic-roughness workflow and classic Blinn-Phong (chosen automatically
  per material), and every material map Assimp/glTF can provide: base
  color/diffuse, normal, metalness, roughness, emissive, specular, and height.
  Falls back to the imported material's `Mat_Ka`/`Mat_Kd`/`Mat_Ks` colors for
  any channel with no texture. Single render pass, one light.
- `legacy/*.glsl` — the shaders `mega_material.glsl` consolidates
  (`bone_animation.glsl`, `bone_animation_bump_mapping.glsl`,
  `bone_animation_material_only.glsl`, `bump_mapping.glsl`,
  `material_pixel_lighting.glsl`, `pbr_animation.glsl`), kept as reference
  examples of each individual feature in isolation.

**Category 3 — stylized/artistic Assimp materials**:

- `toon_material.glsl` — cel/toon shading: discrete lighting bands
  (`toonBands`) over the same data pipeline (bones + material maps) as
  `mega_material.glsl`.
- `rim_lighting_material.glsl` — Fresnel-style rim/silhouette highlight
  (`rimColor`/`rimPower`) over the same data pipeline.
- `legacy/pbr_animation_artistic.glsl` — earlier artistic PBR variant with
  extra user-editable "art direction" uniforms (`artMetallicBoost`,
  `artRoughnessBoost`, `artRoughnessBias`, `artSpecularIntensity`,
  `artAlbedoTint`, `artAmbientBoost`, `artOpacityMultiplier`,
  `artNormalStrength`, `artEmissiveBoost`); kept as reference, not merged into
  the newer stylized shaders.

**Out of scope** (see [assets/shaders/README.md](assets/shaders/README.md) for
details): baked ambient occlusion is feasible without a render-architecture
change and is a candidate for a future spec; real-time SSAO and a
Bokeh/Depth-of-Field shader both require a sampleable depth attachment and a
second post-process pass, which the single-framebuffer/single-pass
`PreviewRenderer` does not currently support.


### Keyframe animation and cameras

Imported models expose their Assimp animation clips in the `Render` panel's
`Animation` combo (`None` keeps the bind pose). The `Loop animation` checkbox
selects whether the clip restarts when its duration elapses or holds its final
keyframe.

Two kinds of animation are supported and handled differently:

- **Skeletal animation** is uploaded as `gBones[]`, exactly like Phoenix.
- **Node (object) animation** — a mesh that moves without a skeleton — is folded
  into that mesh's `model` matrix from its scene node's animated world
  transform, so `model` and `MVP` are per-mesh for such models. Skinned models
  are excluded from this, since `gBones` already bakes the node hierarchy in and
  applying it twice would double-transform the vertices.

The `Camera` combo selects which camera drives `view`, `projection` and
`uCameraPos`:

- `Free camera` (default) uses the interactive orbit/pan/zoom camera.
- Any other entry binds a camera authored inside the model. If that camera's
  node is keyframe-animated, the matrices and `uCameraPos` follow it every
  frame. While a model camera is active, orbit/pan/zoom input is ignored so the
  free camera's framing is preserved and restored untouched when you switch
  back.

Models without cameras show only `Free camera` plus a note saying the model has
no cameras.

`assets/models/KeyframeSamples/` contains two ready-to-use samples: a scene with
keyframe-animated objects and a scene with a keyframe-animated camera. See its
[README](assets/models/KeyframeSamples/README.md).

Orbit, pan and zoom move the **free camera** only; they never modify object
placement. Imported models always render at the transform authored in the file,
so a scene camera shows exactly the framing its author intended regardless of
how the free camera was moved beforehand.

### Instanced scenes

Model files frequently reference a single mesh from many scene nodes — either
because the scene reuses props, or because Assimp's `aiProcess_FindInstances`
merged identical meshes during import. ShaderEditor draws one instance per
`(mesh, node)` reference, so every copy appears at its own node transform. A
city scene made of repeated props may therefore issue several thousand draws
from only a few hundred distinct meshes.

To keep such scenes interactive, the renderer evaluates the node hierarchy once
per frame (not once per mesh), deduplicates materials at import time, and draws
meshes grouped by material so material uniforms and textures are bound once per
distinct material instead of once per mesh.

### Model info panel

`View > Model info` opens a read-only panel describing the currently loaded
model: name and source path, mesh/vertex/triangle/index/material counts, texture
slots by type and embedded-image count, skeleton and bone counts, every
animation clip with its duration and channel count, the model's cameras and
whether each is animated, and the bounding box and radius. The panel refreshes
automatically each time a model is loaded.

## Project Layout

```text
src/      Application, workspace, rendering, and UI code
assets/   Bundled GLSL shader assets
tests/    Catch2 unit tests
specs/    Archived Spec Kit feature artifacts
```

## Dependencies

- CMake 3.27+
- A C++20 compiler
- OpenGL-capable desktop environment
- vcpkg with the following manifest dependencies:
  - `glad`
  - `glfw3`
  - `imgui[docking-experimental,glfw-binding,opengl3-binding]`
  - `glm`
  - `stb`
  - `assimp`

### Install vcpkg

If vcpkg is not already installed, clone it and bootstrap it once:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\tools\vcpkg
& C:\tools\vcpkg\bootstrap-vcpkg.bat
```

Set `VCPKG_ROOT` in each PowerShell session before configuring the project.
Use the existing installation path if vcpkg is already installed:

```powershell
$env:VCPKG_ROOT = "C:\tools\vcpkg"
```

The CMake preset uses this variable to locate the vcpkg toolchain. Dependencies
declared in `vcpkg.json` are installed automatically during configuration.

### Static library linking

The project links the vcpkg dependencies statically. The `default` CMake preset
uses the `x64-windows-static-md` triplet and sets `BUILD_SHARED_LIBS=OFF`, so the
executable does not require vcpkg DLLs such as `glfw3.dll` or `assimp-*.dll` at
runtime. The `-md` suffix intentionally keeps the MSVC runtime dynamic; Windows
system DLLs and the Visual C++ Redistributable are still runtime requirements.

If the build directory was previously configured with the dynamic
`x64-windows` triplet, remove it before configuring again so CMake and vcpkg do
not reuse the old library selection:

```powershell
Remove-Item -Recurse -Force build-vcpkg
cmake --preset default
```

## Build

### Configure

```powershell
$env:VCPKG_ROOT = "C:\tools\vcpkg"
cmake --preset default
```

Run the command from the repository root. If CMake reports that the vcpkg
toolchain cannot be found, verify that `VCPKG_ROOT` points to the directory
containing `scripts\buildsystems\vcpkg.cmake`, then configure again.

### Debug

```powershell
cmake --build --preset default
ctest --preset default
```

Launch:

```powershell
build-vcpkg\Debug\shader_editor.exe
```

### Release

```powershell
cmake --build build-vcpkg --config Release
ctest --test-dir build-vcpkg -C Release --output-on-failure
```

Launch:

```powershell
build-vcpkg\Release\shader_editor.exe
```

### Visual Studio Code

Set `VCPKG_ROOT` before starting Visual Studio Code so the bundled CMake tasks
and the CMake Tools extension inherit it:

```powershell
$env:VCPKG_ROOT = "C:\tools\vcpkg"
code .
```

Then select the `default` configure preset and run `cmake-build` or `ctest`
from the Command Palette. The `Run Shader Editor` launch configuration builds
the Debug target before starting the application.

## How To Use

1. Launch the application.
2. Load the bundled example shaders or open a local Phoenix `.glsl` file.
3. Edit shader source in the `Shader Editor` panel.
4. Recompile with `Ctrl+Enter` or `Update Shader`.
5. Use `Open Model...` from the File menu or the Render panel to import a
   model. Use the `model` button to switch back to an imported model after
   selecting a primitive.
6. When the model contains animations, choose a clip from the `Animation`
   selector or choose `None` to show its bind pose.
7. Drag with the mouse over the preview:
   - Left button: orbit
   - Right button: pan
8. Edit user-controlled uniforms in the `Uniforms` panel. Engine-provided
   uniforms are documented in `Shader Help` and are not editable.
9. Check `Diagnostics` and `Shader Errors` when file, compile, or render issues occur.
10. Rearrange the dockable panels to fit the current workflow.

## Current Validation

The automated validation flow currently covers:

- `cmake --build --preset default`
- `ctest --preset default`
- `cmake --build build-vcpkg --config Release`
- `ctest --test-dir build-vcpkg -C Release --output-on-failure`

Manual validation steps are documented in
[`specs/archive/001-shader-editor/quickstart.md`](specs/archive/001-shader-editor/quickstart.md).
