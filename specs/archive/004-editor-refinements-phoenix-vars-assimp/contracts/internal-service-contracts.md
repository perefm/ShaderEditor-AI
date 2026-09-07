# Phase 1 Contracts: Editor Menu Refinements, Phoenix Auto-Uniforms, and Assimp Model Import

**Phoenix reference commit**: `Spontz/Phoenix` @
`75aff215bfb6ee8d18d6c1967e0635ab49eb9c2d` (tag `v4.2.4`, 2026-08-27). All
Phoenix-derived names in this document (texture uniform names, material
color uniforms, vertex attribute names, `gBones`) come from that commit's
`Mesh.cpp`, `Material.h`/`.cpp`, `Model.cpp`, and `ShaderVars.h`.

This project is a native desktop app with no network API, so "contracts" here
are the internal C++ service/interface signatures that other components and
tests depend on. Each is a "must implement exactly this" surface for the
corresponding user story.

## 1. `ShaderFileService::saveAs` (User Story 1 - Save As)

```cpp
// services/files/ShaderFileService.h
class ShaderFileService {
  public:
    // existing members unchanged...

    // Writes document.source to newPath, then updates document.shaderPath to newPath
    // (clearing vertexPath/fragmentPath, which are unused for single-file Phoenix documents)
    // and calls document.markSaved(). Throws/reports the same way `save()` does on I/O failure.
    void saveAs(ShaderPairDocument& document, const std::filesystem::path& newPath) const;
};
```

Caller contract (`WorkspaceController`):

```cpp
// app/workspace/WorkspaceController.h
bool saveShadersAs(const std::filesystem::path& newPath);
```

- Returns `false` and leaves the document untouched on I/O failure (mirrors
  existing `saveShaders()` failure handling).
- On success, `editorState().document().shaderPath == newPath` and
  `isDirty == false`.

## 2. Menu label contract (User Story 2 - Renames)

No new interface; `Application::drawMainMenu()` string literals change only.
Contract is purely textual: the File menu MUST contain items with the exact
labels `"Save shader"` (shortcut `Ctrl+S`) and `"Update Shader"` (shortcut
`Ctrl+Enter`), each still bound to `workspace_.saveShaders()` and
`shaderEditorPanel_.pressUpdateButton()` respectively, plus a new
`"Save As"` item bound to the Save-As flow (User Story 1).

## 3. `PlaybackClockState` (User Story 3 - Phoenix auto-uniforms)

```cpp
// app/workspace/PlaybackClockState.h
namespace shadereditor {
class PlaybackClockState {
  public:
    void play();
    void pause();
    void reset();                       // elapsedSeconds = 0; play state/tend/bpm untouched
    void advance(float deltaSeconds);    // no-op if !isPlaying()

    void setSectionDurationSeconds(float value);
    void setBpm(float value);

    [[nodiscard]] bool isPlaying() const;
    [[nodiscard]] float elapsedSeconds() const;         // -> "t"
    [[nodiscard]] float sectionDurationSeconds() const; // -> "tend"
    [[nodiscard]] float bpm() const;
    [[nodiscard]] float beat() const; // bpm <= 0 ? 0 : fract(elapsedSeconds * bpm / 60), always [0, 1)
};
}  // namespace shadereditor
```

`WorkspaceController` contract additions:

```cpp
// app/workspace/WorkspaceController.h
void playPreview();
void pausePreview();
void resetPreview();
void setSectionDuration(float seconds);
void setBpm(float bpm);
[[nodiscard]] const PlaybackClockState& playbackClock() const;
```

`UniformIntrospectionService::discover` contract addition: for any uniform
declaration in the shader source whose name is exactly `t`, `tend`, or
`beat` and whose GLSL type is a scalar `float`, the returned
`UniformDefinition` MUST have `provenance = UniformProvenance::PhoenixAuto`
and `editable = false`. Any other type for those names MUST NOT set
`PhoenixAuto` (falls through to normal user-uniform handling, satisfying
FR-013).

`PreviewRenderer::applyUniforms` contract addition: for every
`UniformDefinition` with `provenance == UniformProvenance::PhoenixAuto` and
name `t`/`tend`/`beat`, the uploaded GL uniform value MUST come from the
current `PlaybackClockState` snapshot passed in that frame's `renderFrame`
call, overriding any stale value present in `RenderSession::uniformValues`
for that name.

## 4. `AssimpModelLoader` (User Story 4 - Model import)

```cpp
// rendering/models/AssimpModelLoader.h
namespace shadereditor {
struct ModelLoadResult {
    bool success {false};
    std::string errorMessage;
    ModelDocument document;
};

class AssimpModelLoader {
  public:
    ModelLoadResult load(const std::filesystem::path& modelPath) const;
};
}  // namespace shadereditor
```

Contract details:
- MUST apply Assimp post-process flags including `aiProcess_Triangulate`,
  `aiProcess_GenSmoothNormals`, `aiProcess_CalcTangentSpace`, and
  `aiProcess_SplitByBoneCount` (plus the additional flags listed in
  research.md item 5) so oversized-bone meshes are split rather than
  rejected.
- MUST name texture uniform slots using the pattern
  `"texture_" + phoenixTypeName + std::to_string(indexWithinType + 1)` where
  `phoenixTypeName` is exactly one of: `diffuse`, `specular`, `ambient`,
  `height`, `normal`, `emissive`, `roughness`, `shininess`,
  `ambientoclussion`, `metalness`, `unknown`, `none` (verbatim spellings,
  including the non-standard `ambientoclussion`), matching FR-015.
- MUST populate `ModelMaterial::colorAmbient/colorDiffuse/colorSpecular/specularStrength`
  from `AI_MATKEY_COLOR_AMBIENT` / `AI_MATKEY_COLOR_DIFFUSE` /
  `AI_MATKEY_COLOR_SPECULAR` / `AI_MATKEY_SHININESS_STRENGTH` respectively,
  matching FR-016.
- MUST populate `ModelVertex::boneIds`/`boneWeights` (up to 4 bones per
  vertex, matching Phoenix's `NUM_BONES_PER_VERTEX`) whenever the mesh has
  bone data, matching FR-018.
- On any failure (unreadable file, unsupported format, missing referenced
  texture that Assimp cannot resolve), MUST return
  `{ success = false, errorMessage = <description> }` without throwing, so
  `WorkspaceController::openModel` can report it via `DiagnosticsState`
  (FR-019) and leave the current render target unchanged.

`WorkspaceController` contract additions:

```cpp
// app/workspace/WorkspaceController.h
bool openModel(const std::filesystem::path& modelPath); // sets renderTargetKind = Model on success
void selectModel();     // re-activates the previously loaded model without re-import
// selectPrimitive(...) (existing) sets renderTargetKind back to Primitive
```

`SkeletalAnimator` contract:

```cpp
// rendering/models/SkeletalAnimator.h
namespace shadereditor {
class SkeletalAnimator {
  public:
    // Returns one glm::mat4 per bone (size == document.boneCount), in bone-index order,
    // matching Phoenix's gBones upload order. Returns identity matrices if !document.hasSkeleton.
    std::vector<glm::mat4> boneTransforms(const ModelDocument& document,
                                          float elapsedSeconds,
                                          int animationIndex = 0) const;
};
}  // namespace shadereditor
```

`PreviewRenderer` contract addition: when `RenderSession::renderTargetKind
== Model`, `renderFrame` MUST, for each `ModelMesh`: bind its texture slots to
the exact `shaderUniformName`s recorded on the material; upload
`Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght` if declared by the shader; upload
`gBones` (from `SkeletalAnimator::boneTransforms`) if the shader declares it
and the model has a skeleton; and issue one draw call per mesh — mirroring
Phoenix's `Model::Draw`/`Mesh::Draw`/`Mesh::setMaterialShaderVars`.

## 5. Engine-owned transform and UI contracts

`PreviewRenderer` uploads `MVP`, `model`, and `uCameraPos` automatically.
`UniformIntrospectionService` excludes these names from editable uniform
discovery. `model` is the orbit-only matrix exposed by
`PreviewCamera::modelMatrix()`.

`WorkspaceController` exposes `modelAnimationNames()`,
`selectAnimation(int)`, and `selectedAnimationIndex()`. The Render panel
provides an `Open Model...` action and an animation combo with a `None`
bind-pose entry.

`ModelTextureSlot` supports either an external `sourcePath` or encoded
`embeddedImageData`; `PreviewRenderer` decodes and caches both forms.

The Shader Help panel documents all engine-provided uniforms, Phoenix vertex
attributes, material names, texture slot names, and `gBones`.

**Contract status**: implemented and closed 2026-09-07.
