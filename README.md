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
- Bundled model shader examples, including `bone_animation.glsl`,
  `bone_animation_material_only.glsl`, `bump_mapping.glsl`,
  `bone_animation_bump_mapping.glsl`, `material_pixel_lighting.glsl`,
  `pbr_animation.glsl`, and `pbr_animation_artistic.glsl`

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
  `materialOpacity`: active mesh glTF metallic-roughness/transmission factors
  (`pbr_animation*.glsl`).
- `uniform bool hasPbrTextures`, `hasDiffuseTexture`, `hasNormalMap`,
  `hasEmissiveTexture`: whether the active mesh has a dedicated
  metallic-roughness, base color, normal, or emissive texture bound, so a
  shader can fall back to the scalar factors/colors above when it does not.
- `uniform vec3 emissiveFactor`: active mesh glTF emissive color
  (`pbr_animation*.glsl`).

The bundled `assets/models/NormalTangentTest/NormalTangentTest.glb` is a
royalty-free Khronos CC0 sample with embedded normal maps. Use it with
`bump_mapping.glsl` to validate tangent-space bump mapping. The
`bone_animation_bump_mapping.glsl` example combines the same normal mapping
with Phoenix-compatible skinning and can also be used with animated models.
See "Bundled shaders" below for a description of every shader shipped with
the project.

Imported model vertex attributes follow the Phoenix mesh layout:
`aPos` (0), `aNormal` (1), `aTexCoords` (2), `aTangent` (3),
`aBiTangent` (4), `aBoneID` (5), and `aBoneWeight` (6). Built-in primitives
provide `aPos` and `aUv`.

Declare and use these names in shader stages that need them. Other uniforms
declared by the shader are discovered after a successful compile and remain
editable from the `Uniforms` panel.

### Bundled shaders

All bundled shaders live in `assets/shaders/`. Each is a single `.glsl` file
with a `#type vertex` and `#type fragment` section.

**Primitive-only examples** (built-in plane/cube/torus/sphere/cylinder,
`aPos`/`aUv` vertex layout only):

- `basic.glsl` — flat-shaded solid color from a single `uniform vec3 color`;
  the minimal starting point for a new shader.
- `uniforms.glsl` — demonstrates editable `float`/`vec4` uniforms
  (`intensity`, `tint`) with no lighting.
- `diagnostics.glsl` — outputs solid white; useful for isolating whether a
  problem is in geometry/transform setup or in shading.
- `textured.glsl` — samples a single `sampler2D imageTexture` and outputs it
  unlit.
- `pixel_lighting.glsl` — per-pixel ambient + diffuse + specular lighting
  using a face normal derived from screen-space derivatives (`dFdx`/`dFdy`),
  so it works on primitives that carry no vertex normal attribute.

**Imported-model examples** (Phoenix mesh vertex layout: `aPos`, `aNormal`,
`aTexCoords`, `aTangent`, `aBiTangent`, `aBoneID`, `aBoneWeight`):

- `bone_animation.glsl` — Phoenix-compatible skeletal (bone) animation with
  Blinn-Phong lighting and a diffuse texture; the general-purpose default for
  animated imported models.
- `bone_animation_material_only.glsl` — same skinning as `bone_animation.glsl`
  but samples no textures at all; shading comes entirely from the imported
  material's `Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght` colors. Useful for
  animated models with no usable texture files.
- `bump_mapping.glsl` — static (unskinned) tangent-space normal mapping plus
  Blinn-Phong lighting; validate with
  `assets/models/NormalTangentTest/NormalTangentTest.glb`.
- `bone_animation_bump_mapping.glsl` — combines the same tangent-space normal
  mapping with Phoenix-compatible skinning, for animated models with normal
  maps.
- `material_pixel_lighting.glsl` — unskinned counterpart to
  `bone_animation_material_only.glsl`: no `gBones` skinning path (suits
  static models and models animated by node keyframes), no texture sampling,
  Blinn-Phong lighting evaluated per pixel in world space from the imported
  material colors alone. Applying it to a skinned model renders that model in
  its bind pose.
- `pbr_animation.glsl` — physically-based (Cook-Torrance metallic-roughness)
  skinned shader for glTF-style imports. Reads the imported material's base
  color/metallic/roughness/transmission/opacity/normal/emissive values
  (falling back to scalar factors when a mesh has no dedicated texture for
  one of them) and blends translucent materials (e.g. glass) correctly
  against the rest of the scene. All of its material-driven uniforms are
  engine-supplied and read-only in the `Uniforms` panel — this shader is a
  physically-accurate reference/baseline, not meant to be hand-tuned per
  material.
- `pbr_animation_artistic.glsl` — same physically-based baseline as
  `pbr_animation.glsl`, plus extra ordinary (user-editable) "art direction"
  uniforms layered on top: `artMetallicBoost`, `artRoughnessBoost`,
  `artRoughnessBias`, `artSpecularIntensity`, `artAlbedoTint`,
  `artAmbientBoost`, `artOpacityMultiplier`, `artNormalStrength`, and
  `artEmissiveBoost`. Use this variant to push a model's look for stylistic
  reasons without hand-editing the source material; at every control's
  neutral default (1.0/0.0/white) the render is identical to
  `pbr_animation.glsl`.

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
