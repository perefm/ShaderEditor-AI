# Phase 1 Data Model: Editor Menu Refinements, Phoenix Auto-Uniforms, and Assimp Model Import

## PlaybackClockState (new)

Owned by `WorkspaceController`. Value-type state analogous to
`PreviewInteractionState`.

| Field | Type | Notes |
|---|---|---|
| `isPlaying` | `bool` | Default `true`. Toggled by Play/Pause UI actions. |
| `elapsedSeconds` | `float` | The `t` uniform value. Advances by frame delta time while `isPlaying`. Never negative. |
| `sectionDurationSeconds` | `float` | The `tend` uniform value. User-editable at any time; independent of `isPlaying`. Default e.g. `60.0F`. |
| `bpm` | `float` | User-editable. Default e.g. `120.0F`. May be set to 0 or negative by the user. |

Derived:
- `beat() const -> float`: returns `0.0F` if `bpm <= 0.0F`, else
  `fract(elapsedSeconds * bpm / 60.0F)`, always in `[0, 1)`.

Behaviors:
- `play()`, `pause()`, `reset()` (`elapsedSeconds = 0.0F`, does not change `isPlaying`/`sectionDurationSeconds`/`bpm`).
- `advance(float deltaSeconds)`: `if (isPlaying) elapsedSeconds += deltaSeconds;` no-op when paused.

## UniformDefinition (extended)

Existing struct in `rendering/shaders/UniformDefinition.h` gains one new
field to distinguish auto-supplied uniforms from user-editable ones without
overloading the existing `editable` flag's current meaning (which today may
be used for other validation purposes):

| Field | Type | Notes |
|---|---|---|
| `provenance` (new) | `enum class UniformProvenance { User, PhoenixAuto }` | Default `User`. Set to `PhoenixAuto` by `UniformIntrospectionService` when the declared name/type matches one of the recognized auto-uniforms (`t`, `tend`, `beat` as `float`; `Mat_Ka`, `Mat_Kd`, `Mat_Ks` as `vec3`; `Mat_KsStrenght` as `float`; `gBones` as `mat4[]`). |
| `editable` (existing) | `bool` | Set to `false` when `provenance == PhoenixAuto`, matching FR-012's "no manual editing controls" requirement. |

Recognized auto-uniform table (name → expected GLSL type → data source):

| Name | GLSL type | Source |
|---|---|---|
| `t` | `float` | `PlaybackClockState::elapsedSeconds` |
| `tend` | `float` | `PlaybackClockState::sectionDurationSeconds` |
| `beat` | `float` | `PlaybackClockState::beat()` |
| `Mat_Ka` | `vec3` | Active mesh's material ambient color |
| `Mat_Kd` | `vec3` | Active mesh's material diffuse color |
| `Mat_Ks` | `vec3` | Active mesh's material specular color |
| `Mat_KsStrenght` | `float` | Active mesh's material specular strength (Phoenix's verbatim, misspelled name) |
| `gBones` | `mat4[]` | `SkeletalAnimator`'s current per-frame bone transform array |
| `texture_<type><N>` | `sampler2D` | Bound GPU texture handle for that material texture slot |

If a shader declares one of these names with an incompatible type (e.g.
`uniform vec3 t;`), `UniformIntrospectionService` treats it as an ordinary
(non-auto) uniform mismatch, surfaced through the existing type-mismatch
diagnostic path (FR-013) — it is NOT coerced into `PhoenixAuto` provenance.

## RenderSession (extended)

Existing struct in `rendering/shaders/RenderSession.h`.

| Field | Type | Notes |
|---|---|---|
| `renderTargetKind` (new) | `enum class RenderTargetKind { Primitive, Model }` | Default `Primitive`, preserving existing behavior. |
| `selectedPrimitiveId` (existing) | `std::string` | Meaningful when `renderTargetKind == Primitive`. |
| `loadedModelId` (new) | `std::string` | Identifier (e.g. file path or stem) of the currently loaded model, meaningful when `renderTargetKind == Model`; empty if no model has ever been loaded. |
| `playback` (new) | `PlaybackClockState` (or a read-only snapshot of it) | Exposed so the UI (Uniforms panel, transport controls) can display current `t`/`tend`/`bpm`/`beat`/play-state without reaching into `WorkspaceController` internals. |

## ModelDocument (new)

Engine-agnostic representation produced by `AssimpModelLoader`, consumed by
`PreviewRenderer`. Lives in `rendering/models/ModelDocument.h`. Contains no
Assimp types.

| Field | Type | Notes |
|---|---|---|
| `meshes` | `std::vector<ModelMesh>` | One entry per (possibly split) mesh. |
| `hasSkeleton` | `bool` | True if any mesh has bone data. |
| `boneCount` | `std::size_t` | Total distinct bones across the model, used to size the `gBones` upload array. |
| `sourcePath` | `std::filesystem::path` | The file the model was imported from. |
| `boundsMin` / `boundsMax` | `glm::vec3` | Bind-pose AABB used to scale model camera framing and interaction. |

### ModelMesh

| Field | Type | Notes |
|---|---|---|
| `vertices` | `std::vector<ModelVertex>` | Layout matches Phoenix's `Mesh::setupMesh` attribute order. |
| `indices` | `std::vector<unsigned int>` | Triangle list. |
| `material` | `ModelMaterial` | Textures + color/scalar properties for this mesh. |

### ModelVertex

| Field | Type | GLSL attribute name (verbatim, Phoenix parity) |
|---|---|---|
| `position` | `glm::vec3` | `aPos` |
| `normal` | `glm::vec3` | `aNormal` |
| `texCoords` | `glm::vec2` | `aTexCoords` |
| `tangent` | `glm::vec3` | `aTangent` |
| `biTangent` | `glm::vec3` | `aBiTangent` |
| `boneIds` | `std::array<uint32_t, 4>` | `aBoneID` |
| `boneWeights` | `std::array<float, 4>` | `aBoneWeight` |

### ModelMaterial

| Field | Type | Notes |
|---|---|---|
| `textureSlots` | `std::vector<ModelTextureSlot>` | Each slot carries the Phoenix-style shader uniform name (`texture_diffuse1`, ...), an external path or embedded encoded image bytes, and a lazy GL handle. |
| `colorAmbient` | `glm::vec3` | → `Mat_Ka` |
| `colorDiffuse` | `glm::vec3` | → `Mat_Kd` |
| `colorSpecular` | `glm::vec3` | → `Mat_Ks` |
| `specularStrength` | `float` | → `Mat_KsStrenght` |

### ModelTextureSlot

| Field | Type | Notes |
|---|---|---|
| `shaderUniformName` | `std::string` | E.g. `"texture_diffuse1"`, produced per FR-015 naming rule. |
| `sourcePath` | `std::filesystem::path` | Resolved texture file path on disk. |
| `embeddedImageData` | `std::vector<unsigned char>` | Encoded image bytes copied from an embedded Assimp texture (for example a `.glb`). |
| `glTextureId` | `GLuint` | Populated by `PreviewRenderer` when the texture is uploaded (0 until then). |

## SkeletalAnimator (new)

Stateless-ish helper (or small stateful class holding cached bone offset
matrices) that, given a `ModelDocument` and the current playback time
(`PlaybackClockState::elapsedSeconds`), computes the current array of
`glm::mat4` bone transforms to upload as `gBones`. Not persisted in
`RenderSession`; recomputed each frame by `PreviewRenderer` from
`ModelDocument` + `PlaybackClockState` + the selected animation index.

## ShaderPairDocument (unchanged shape, new usage)

No new fields required. "Save As" reassigns `shaderPath` to the new path
(this field already exists) and calls the existing `markSaved()`. This keeps
`ShaderPairDocument` as the single existing entity representing "which file
is the active shader," per the spec's requirement that subsequent Save/Update
operate on the new path.

## State transitions summary

- **Save As**: `ShaderPairDocument.shaderPath` (old) → user picks new path →
  `ShaderFileService::saveAs` writes file → `shaderPath` (new) +
  `isDirty = false`. Cancel leaves all fields unchanged.
- **Playback**: `PlaybackClockState.isPlaying` toggles via Play/Pause;
  `elapsedSeconds` monotonically increases while playing, frozen while paused,
  reset to 0 via Reset; `sectionDurationSeconds`/`bpm` are independently
  user-editable at any time.
- **Model load**: no model loaded → `openModel(path)` → on success,
  `RenderSession.renderTargetKind = Model`, `ModelDocument` cached in
  `WorkspaceController`; on failure, `DiagnosticsState` gets an error entry
  and `RenderSession` is unchanged (previous target, primitive or prior
  model, remains active).
- **Switch to primitive**: `selectPrimitive(id)` sets
  `RenderSession.renderTargetKind = Primitive` and
  `selectedPrimitiveId = id`; the cached `ModelDocument` (if any) is retained
  in `WorkspaceController` for later reselection via a new `selectModel()`
  action (no re-import).
- **Animation selection**: `selectedAnimationIndex == -1` produces the bind
  pose; a valid index samples that `AnimationClip` in a loop. New models
  default to their first clip when one exists.

## Closure

Updated 2026-09-07 to reflect the implemented model bounds, embedded texture
storage, animation selection, engine-owned transform uniforms, and normalized
beat phase. This data model is final for spec 004.
