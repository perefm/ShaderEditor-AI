# Implementation Plan: Editor Menu Refinements, Phoenix Auto-Uniforms, and Assimp Model Import

**Branch**: `004-editor-refinements-phoenix-vars-assimp` | **Date**: 2026-09-07 | **Spec**: [spec.md](/C:/CODE/ShaderEditor-AI/specs/004-editor-refinements-phoenix-vars-assimp/spec.md)
**Input**: Feature specification from `/specs/004-editor-refinements-phoenix-vars-assimp/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

**Phoenix Reference Commit**: `Spontz/Phoenix` @
[`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d`](https://github.com/Spontz/Phoenix/commit/75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d)
(tag `v4.2.4`, 2026-08-27). See [research.md](/C:/CODE/ShaderEditor-AI/specs/004-editor-refinements-phoenix-vars-assimp/research.md)
section 5 for the exact files consulted. If Phoenix is updated later, diff
those files against the new revision before extending this feature further.

## Summary

Four related changes to the ShaderEditor desktop app: (1) add a "Save As"
File-menu action that writes the current unified Phoenix shader document to a
new path and re-targets the active document to it; (2) rename two existing
File-menu items ("Save Current Shaders" → "Save shader", "Update Shaders" →
"Update Shader") with no behavior change; (3) introduce a playback clock
(Play/Pause/Reset, editable "section duration" `tend`, editable `bpm`) that
automatically supplies `t`, `tend`, and `beat` uniforms to any shader that
declares them, without exposing manual edit controls for those uniforms in
the Uniforms panel; (4) add Assimp-based 3D model import that replaces the
active preview primitive with a real mesh, binding per-material textures and
color/scalar properties using Phoenix's exact uniform naming
(`texture_<type><N>`, `Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght`) and
uploading skeletal animation as a `gBones` mat4 array driven by the same
playback clock, with oversized-bone meshes split via Assimp's
`aiProcess_SplitByBoneCount`, matching Phoenix's `drawScene` section
behavior; (5) bundle runtime example assets proving (3) and (4) work
out of the box: a bone-animation example shader, a bump-mapping example
shader, a PBR (metallic-roughness) animation example shader, a small
royalty-free animated/textured sample model (`assets/models/Fox/Fox.glb`),
and a small royalty-free animated sample model with a
`pbrMetallicRoughness` material (`assets/models/CesiumMan/CesiumMan.glb`).

## Technical Context

**Language/Version**: C++20 (existing codebase; MSVC/Windows toolchain via CMake + vcpkg)
**Primary Dependencies**: OpenGL 4.6 core, GLFW, Dear ImGui (docking), GLM, glad, stb_image, imgui_color_text_edit (vendored in `third_party/`), native Win32 file dialogs (`comdlg32`); **new**: Assimp (added via vcpkg)
**Storage**: Local filesystem only — `.glsl` shader files (existing) and model files (new: `.fbx`, `.gltf`/`.glb`, `.obj`, `.dae`, plus any other format Assimp supports) with their referenced texture files on disk
**Testing**: CTest via `add_subdirectory(tests)`, GoogleTest/Catch-style unit tests already present under `tests/unit` (existing pattern: `test_*.cpp` per service/component); manual verification steps for OpenGL/UI behavior per constitution
**Target Platform**: Windows desktop (existing constraint — native file dialogs are Windows-only today; this feature does not add new cross-platform requirements beyond what already exists)
**Project Type**: Single desktop application (`src/` + `tests/`, existing structure)
**Performance Goals**: Maintain interactive frame rates in the Render View (no perceptible stutter) while a playback clock updates every frame and while an imported model with a moderate animated rig (order of tens of bones, tens of thousands of vertices) is rendered
**Constraints**: Must not block the UI thread for more than a brief, expected duration while importing a model (parsing + texture loads); must not regress existing shader open/save/uniform workflows; auto-uniforms must never be user-editable in the Uniforms panel
**Scale/Scope**: Single active model + single active shader document at a time; no multi-model scenes, no camera/light import from the model file, no model export/save

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Platform compatibility**: Feature stays within the already-documented
  Windows-only native dialog constraint (Open Shader/Open Image already work
  this way). Assimp itself is cross-platform, but the new "Open Model..."
  dialog reuses the existing Win32 `openFileDialog` helper, so this feature
  does not regress or expand cross-platform claims — it is consistent with
  existing behavior, not a new violation.
- **Verification**: Each user story gets targeted unit tests where testable
  without a live GL context (uniform auto-population math, `beat` clamping,
  Save As path handling, Assimp material/texture-name mapping, bone-index
  mapping) plus a manual verification checklist for on-screen behaviors
  (Play/Pause/Reset visual effect, model rendering, animation playback).
- **UI consistency**: "Save As" reuses the existing native save/open dialog
  pattern; menu renames are label-only; playback controls (Play/Pause/Reset,
  `tend`, `bpm`) are added using existing ImGui panel conventions (e.g.,
  alongside the Uniforms panel or Render View controls, mirroring how preview
  interaction controls are already presented); auto-uniforms are visually
  marked read-only in the Uniforms panel rather than introducing a new
  interaction pattern.
- **OpenGL/runtime impact**: Model import adds new GPU resources (VAOs/VBOs
  per mesh, textures per material, bone matrix upload) inside
  `PreviewRenderer`, following the same lazy-recreate pattern already used
  for primitives (`ensureMesh`/`ensureFramebuffer`); loading happens on the
  main thread but is bounded to file parse + texture decode (no long-running
  background work introduced in this iteration); failures produce
  diagnostics via the existing `DiagnosticsState` instead of exceptions
  escaping to the render loop.

## Project Structure

### Documentation (this feature)

```text
specs/004-editor-refinements-phoenix-vars-assimp/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
assets/
├── shaders/
│   ├── bone_animation.glsl          # NEW: example shader for Assimp skinned-mesh animation (gBones, aBoneID/aBoneWeight)
│   ├── bump_mapping.glsl            # NEW: example shader demonstrating bump/normal mapping
│   └── pbr_animation.glsl           # NEW: example shader combining skinning with metallic-roughness PBR shading
└── models/                          # NEW subdirectory
    ├── Fox/
    │   ├── Fox.glb                  # NEW: bundled sample rigged/textured/animated model (Khronos glTF-Sample-Assets, CC0 + CC-BY)
    │   └── README.md                # NEW: license/attribution notice for Fox.glb
    └── CesiumMan/
        ├── CesiumMan.glb            # NEW: bundled sample rigged/textured/animated PBR model (Khronos glTF-Sample-Assets, CC-BY 4.0)
        └── README.md                # NEW: license/attribution notice for CesiumMan.glb

src/
├── app/
│   ├── application/
│   │   └── Application.cpp          # File menu: add "Save As", rename 2 items,
│   │                                 # add "Open Model...", wire Play/Pause/Reset UI
│   ├── platform/
│   │   └── WindowContext.*          # unchanged
│   └── workspace/
│       ├── WorkspaceController.*    # add saveShadersAs(path), openModel(path),
│       │                            # own PlaybackClock + SectionTimingSettings,
│       │                            # own the active render target (primitive vs model)
│       ├── PlaybackClockState.{h,cpp}   # NEW: Play/Pause/Reset + elapsed time + tend/bpm/beat calc
│       ├── UniformState.*           # extend to mark auto-uniforms as non-editable
│       ├── PreviewInteractionState.* # unchanged
│       └── DiagnosticsState.*       # unchanged (reused for model-load diagnostics)
├── editor/
│   └── ShaderPairDocument.h         # unchanged (Save As reuses shaderPath swap)
├── rendering/
│   ├── geometry/
│   │   ├── PrimitiveLibrary.*       # unchanged
│   │   └── PreviewPrimitive.h       # unchanged
│   ├── models/                      # NEW subdirectory
│   │   ├── ModelDocument.h          # NEW: meshes, materials, bones, animations (engine-agnostic data)
│   │   ├── AssimpModelLoader.{h,cpp}   # NEW: Assimp import -> ModelDocument, Phoenix-style texture/material naming
│   │   └── SkeletalAnimator.{h,cpp}    # NEW: per-frame bone transform computation ("gBones")
│   ├── opengl/
│   │   ├── PreviewRenderer.*        # extend: render either a primitive or a ModelDocument's meshes,
│   │   │                            # upload gBones/material uniforms, own model GPU resources
│   │   └── PreviewCamera.*          # unchanged
│   └── shaders/
│       ├── UniformDefinition.h      # add `source` (User | AutoPhoenix) or `editable` reuse
│       ├── UniformIntrospectionService.* # recognize t/tend/beat/Mat_*/gBones as auto-uniforms
│       └── RenderSession.h          # add active render-target kind (Primitive | Model) + playback snapshot
├── services/
│   ├── files/
│   │   └── ShaderFileService.*      # add saveAs(document, newPath)
│   └── shaders/
│       └── PhoenixShaderParser.*    # unchanged
└── ui/
    ├── panels/
    │   ├── RenderViewPanel.*        # expose model-open action, playback transport controls
    │   └── UniformsPanel.*          # render auto-uniforms as read-only rows
    └── widgets/
        └── DocumentDialogs.*        # add Save As dialog helper, Open Model dialog helper

tests/
└── unit/
    ├── test_playback_clock.cpp          # NEW: t/tend/beat math, pause/reset, bpm<=0 guard
    ├── test_shader_file_service.cpp     # extend: saveAs behavior
    ├── test_uniform_introspection.cpp   # extend: auto-uniform detection/marking
    ├── test_assimp_model_loader.cpp     # NEW: texture naming, material property mapping,
    │                                    # bone/vertex attribute mapping (using small fixture assets)
    └── test_workspace_models.cpp        # extend: openModel/back-to-primitive round trip
```

**Structure Decision**: Single desktop application project (existing
`src/` + `tests/` layout, CMake `shader_editor_core` static library +
`shader_editor` executable). No new top-level projects. New code is added as
new files inside existing layers (`app/workspace`, `rendering/opengl`,
`services/files`, `ui/panels`) plus one new `rendering/models` subdirectory
for Assimp-specific import logic, keeping Assimp usage isolated from the rest
of the rendering code behind `ModelDocument`/`AssimpModelLoader`.

## Complexity Tracking

> Fill ONLY if Constitution Check has violations that must be justified

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| New `rendering/models/` subsystem (loader + animator) instead of extending `PrimitiveLibrary` | Assimp models carry materials, textures, and skeletal animation that don't fit the existing static-primitive-vertex model; isolating them keeps `PrimitiveLibrary`/`PreviewRenderer` primitive path unchanged (Constitution V: keep changes small/scoped) | Bolting model import directly into `PrimitiveLibrary` would conflate two very different data shapes (static built-in mesh vs. imported animated mesh+material+bones) and risk destabilizing the existing, already-tested primitive rendering path |
