# Phase 0 Research: Editor Menu Refinements, Phoenix Auto-Uniforms, and Assimp Model Import

## 1. Save As implementation approach

**Decision**: Reuse the existing Win32 `openFileDialog` helper family in
`Application.cpp` (used today for "Open Shader..." / "Open Image...") but add
a corresponding **save** dialog helper (`GetSaveFileNameW`) filtered to
`*.glsl`, defaulting to the current document's directory/name when available.
On confirm, call a new `ShaderFileService::saveAs(document, newPath)` that
writes `document.source` to `newPath`, then updates `document.shaderPath`
(and clears `vertexPath`/`fragmentPath`, which are already unused for
single-file Phoenix documents) and calls `document.markSaved()`.

**Rationale**: Matches Constitution III (Simple, Consistent UI) — no new
dialog framework, same look/feel as Open Shader. Keeps `ShaderPairDocument`
as the single source of truth for "which file is this", so Save
(`Ctrl+S`)/Update Shader after a Save As automatically target the new path
with no extra state to track in `WorkspaceController`.

**Alternatives considered**: Building an in-app "Save As" text-input modal —
rejected because it diverges from the existing native-dialog pattern the app
already uses for every other file operation.

## 2. Menu label renames

**Decision**: Pure string literal changes in `Application::drawMainMenu()`
(`src/app/application/Application.cpp`); no signature/behavior changes to
`workspace_.saveShaders()` or `shaderEditorPanel_.pressUpdateButton()`.

**Rationale**: Constitution V (Keep Changes Small) — this is a label-only
change; no reason to touch the underlying methods.

## 3. Playback clock design (`t`, `tend`, `beat`)

**Decision**: Introduce a small `PlaybackClockState` value type (analogous to
existing `PreviewInteractionState`/`UniformState`) owned by
`WorkspaceController`, holding:
- `isPlaying: bool` (default `true` per FR-006/FR-007 semantics — advances
  unless explicitly paused; UI can choose default Play state)
- `elapsedSeconds: float` (`t`)
- `sectionDurationSeconds: float` (`tend`, user-editable, independent of play state)
- `bpm: float` (user-editable)
- Derived `beat()` accessor: `bpm <= 0 ? 0.0f : elapsedSeconds * bpm / 60.0f`

Each rendered frame, `WorkspaceController::renderPreview` advances
`elapsedSeconds` by the frame delta time only when `isPlaying` is true (delta
time computed the same way the existing render loop already tracks frame
timing — reusing whatever timer `Application`'s main loop already has access
to via GLFW, e.g. `glfwGetTime()` deltas). `Reset()` sets `elapsedSeconds` to
0 (does not touch `isPlaying`, `sectionDurationSeconds`, or `bpm`).

`UniformIntrospectionService::discover` is extended to recognize uniform
declarations named exactly `t`, `tend`, `beat` with a scalar `float` type and
mark their `UniformDefinition::editable = false` plus a new
provenance flag (see data-model.md) so the Uniforms panel can render them
distinctly and `PreviewRenderer::applyUniforms` can overwrite their value
from `PlaybackClockState` every frame regardless of what
`UniformState`/`RenderSession::uniformValues` currently holds for that name.

**Rationale**: Keeps the time source in one place (`WorkspaceController`),
consistent with how `PreviewInteractionState` already centralizes
camera/interaction state next to the controller that owns rendering timing.
Computing `beat` as a derived getter (rather than storing it) avoids
divergence/staleness bugs.

**Alternatives considered**: Storing `beat` as its own mutable field updated
imperatively each frame — rejected as an unnecessary extra piece of state to
keep in sync; a pure function of `t`/`bpm` is simpler and cannot drift.

## 4. BPM ≤ 0 guard

**Decision**: `beat()` returns `0.0f` whenever `bpm <= 0.0f`, per FR-010 (this
was already resolved during spec clarification, not re-litigated here).

## 5. Assimp integration and texture/material naming

**Decision**: Add `assimp` as a new vcpkg dependency
(`vcpkg.json` → `"assimp"`), linked into `shader_editor_core` alongside the
existing glad/glfw3/glm/stb dependencies. Introduce `AssimpModelLoader`
(`rendering/models/AssimpModelLoader.{h,cpp}`) that:
1. Calls `Assimp::Importer::ReadFile` with post-process flags mirroring
   Phoenix's `Model.cpp` (`aiProcess_Triangulate`, `aiProcess_GenSmoothNormals`,
   `aiProcess_CalcTangentSpace`, `aiProcess_SplitByBoneCount`,
   `aiProcess_FindInstances`, `aiProcess_ValidateDataStructure`,
   `aiProcess_ImproveCacheLocality`, `aiProcess_RemoveRedundantMaterials`,
   `aiProcess_FindDegenerates`) so bone-count overflow is handled the same way
   Phoenix handles it (mesh splitting), matching the spec's Edge Case
   resolution.
2. For each `aiMesh`, produces vertex data with attributes named/laid out
   like Phoenix's `Mesh::setupMesh` layout: `aPos` (vec3), `aNormal` (vec3),
   `aTexCoords` (vec2), `aTangent` (vec3), `aBiTangent` (vec3), `aBoneID`
   (uint4), `aBoneWeight` (vec4) — exact names reproduced verbatim (FR-018).
3. For each `aiMaterial`, loads textures per `aiTextureType` and assigns
   shader uniform names using Phoenix's exact `Material::loadTextures`
   pattern: `texture_diffuse`, `texture_specular`, `texture_ambient`,
   `texture_height`, `texture_normal`, `texture_emissive`,
   `texture_roughness`, `texture_shininess`, `texture_ambientoclussion`
   (verbatim spelling from Phoenix, including the non-standard spelling),
   `texture_metalness`, `texture_unknown`, `texture_none`, each suffixed with
   a 1-based index per texture of that type (FR-015). Reuses `stb_image`
   (already a dependency) for on-disk texture decode, matching how the
   existing `openImageForUniform` flow already loads images.
4. Also reads `AI_MATKEY_COLOR_DIFFUSE`, `AI_MATKEY_COLOR_AMBIENT`,
   `AI_MATKEY_COLOR_SPECULAR`, `AI_MATKEY_SHININESS_STRENGTH` into
   `Mat_Kd`/`Mat_Ka`/`Mat_Ks`/`Mat_KsStrenght` (FR-016), verbatim names.
5. If the scene has bones, builds a bone-name → index map and, per animation
   frame, computes bone transforms (node hierarchy × animation channel
   interpolation × offset matrix), producing a flat array uploaded as the
   `gBones` `mat4[]` uniform (FR-017), matching Phoenix's `Model::Draw`
   (`shader->setValue("gBones", ...)`).

`ModelDocument` (new, in `rendering/models/ModelDocument.h`) is the
engine-agnostic in-memory representation (meshes + materials + optional
skeleton/animation) that `PreviewRenderer` consumes — it does not expose any
Assimp types outside `AssimpModelLoader`, keeping the Assimp dependency
contained to one translation unit pair, consistent with how
`PhoenixShaderParser` isolates Phoenix-file-format parsing from the rest of
the app.

**Rationale**: FR-015/FR-016/FR-017/FR-018 require byte-for-byte name parity
with Phoenix's own engine so real Phoenix shaders work unmodified; the only
reliable way to guarantee this is to mirror Phoenix's actual source
(`Mesh.cpp`, `Material.cpp`, `Model.cpp`, confirmed via the public
`Spontz/Phoenix` GitHub repository during spec authoring) rather than
inventing a new convention.

**Alternatives considered**: Using Assimp's default `aiTextureType` names or
a custom `u_`-prefixed convention — rejected, would break FR-015/FR-016
parity with Phoenix and defeat the purpose of the feature (running real
Phoenix shaders unmodified).

## 6. Rendering a model instead of a primitive

**Decision**: Extend `RenderSession` with an enum
`RenderTargetKind { Primitive, Model }` and keep `selectedPrimitiveId` as-is
for the `Primitive` case; add `std::optional<std::string> loadedModelPath` or
similar identifier for the `Model` case. `PreviewRenderer::renderFrame`
branches on this kind: primitive path unchanged; model path iterates the
`ModelDocument`'s meshes, binding each mesh's VAO (created once and cached,
mirroring the existing `meshes_` cache keyed by primitive id) and its
material's textures, then issuing one draw call per mesh — mirroring
`Model::Draw`/`Mesh::Draw` in Phoenix. Switching back to a primitive
(`WorkspaceController::selectPrimitive`) simply changes
`RenderSession::renderTargetKind` back to `Primitive`; the loaded
`ModelDocument` is retained in `WorkspaceController` (not destroyed) so
re-selecting "model" as the target (a small new `selectModel()` action) does
not require re-import, per the spec's resolved edge case.

**Rationale**: Keeps the existing `RenderSession`/primitive-selection
contract intact for backward compatibility (existing tests like
`test_render_session.cpp` keep working for the primitive path) while adding
a parallel, clearly separated path for models.

## 7. Diagnostics on model-load failure

**Decision**: `AssimpModelLoader::load` returns a result type
(`ModelLoadResult { bool success; std::string errorMessage; ModelDocument document; }`,
mirroring `PhoenixParseResult`'s existing shape) so
`WorkspaceController::openModel` can report failures through
`DiagnosticsState::addError` exactly like `loadExampleShaders`/`openShader`
already do, and leave the previous render target untouched on failure
(FR-019).

**Rationale**: Reuses the existing, already-tested diagnostics flow instead
of introducing a new error-reporting mechanism (Constitution III/V).

## 8. Performance / non-blocking import

**Decision**: For this iteration, model import runs synchronously on the
main thread (same as today's synchronous shader/image loading), but is scoped
to only the parse + texture decode step, not per-frame work; the spec's
SC-006 requirement ("no UI freeze beyond a brief, expected load time") is
satisfied by keeping sample/test fixture models small (per Success Criteria)
rather than introducing async loading infrastructure, which is out of scope
for this feature per the Assumptions section (no mention of background
loading in the spec).

**Rationale**: Matches Constitution V (Keep Changes Small) — asynchronous
loading would be a much larger architectural change (thread-safety for GL
resource creation) not requested by the spec.
