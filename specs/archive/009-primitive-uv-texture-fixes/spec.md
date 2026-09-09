# Feature Specification: Built-In Primitive UV Mapping and Texture-State Isolation Fixes

**Feature Branch**: `009-primitive-uv-texture-fixes`
**Created**: 2026-09-09
**Status**: Implemented (all user stories confirmed/build-verified)
**Input**: User bug reports (informal Spanish), reported iteratively in one session:
1. "en las coordenadas de textura de los objetos simples, como el cubo. En el cubo debería haber
   las coordenadas de textura como si fuese un dado, de modo que cada cara del cubo represente la
   textura correctamente. Revisa que el resto de objetos tengas las coordenadas de textura
   debidamente generadas."
2. "he visto que cuando se carga un objeto de assimp, se cargan las texturas asociadas (hasta aqui,
   bien). El problema es que si entonces cambio a un objeto como el toro, o la esfera, se
   mantienen las texturas del assimp, esos objetos no deberían tener sus propias texturas (es
   decir, por defecto, ninguna)?"
3. "ahora si cargo una textura en un shader, y despues cargo un modelo, no se ve la textura del
   modelo! se ve la textura que he cargado manualmente." + "en el caso que despues de cargar un
   modelo con assimp, se pase a una primitiva, tampoco espero que se carguen las texturas del
   assimp, sino que cada modelo ha de tener sus propias texturas."
4. "Algunos cambios mas: - Si se cambia entre un objeto de una primitiva, me interesa que se
   mantenga la textura (es decir, si estoy viendo un plano y paso a una esfera, quiero que se
   mantenga la textura). En el caso de que le de al 'modelo' entonces se han de ver las texturas
   del modelo. [...] - Por otro lado, en el plano y en el cubo aparece la textura del revés"
5. "en el toro, esfera y cubo, también se ve la textura del revés"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Cube gets a "dice" UV mapping instead of a spherical projection (Priority: P1) — ✅ Done

As a shader author previewing a manually loaded texture on the built-in cube primitive, I want
each of the cube's six faces to display the full, undistorted texture (like a die face or a crate
texture), instead of a thin sliver of the texture wrapped around the cube as if it were a sphere.

**Why this priority**: The cube's UVs were derived from a spherical projection
(`atan2`/`asin` of the vertex position) applied uniformly to every built-in primitive, which is
visually correct for a sphere but produces a barely recognizable, heavily distorted result on a
cube's flat faces — this was the original, most visible symptom reported.

**Independent Test**: Load a checkerboard or numbered-face texture via "Load Image" onto a
`sampler2D` uniform, select the "Cube" primitive, and confirm every face shows the complete
texture right-side up and undistorted, not a warped fragment of it.

**Acceptance Scenarios**:

1. **Given** the cube primitive, **When** its mesh is generated, **Then** each of its 6 faces gets
   its own full `[0,1]x[0,1]` UV rectangle (a "dice" mapping) rather than sharing one UV formula
   derived from vertex position across the whole shape.
2. **Given** any other built-in primitive (plane, sphere, cylinder, torus), **When** its mesh is
   generated, **Then** its UV mapping is the conventional one for that shape (planar for the
   plane, equirectangular lat/long for the sphere, circumferential+cap-disc for the cylinder,
   toroidal major/minor angle for the torus) — not the cube's per-face mapping and not the old
   one-size-fits-all spherical projection.

---

### User Story 2 - A model's own textures are never shadowed by a stale texture from another render target (Priority: P1) — ✅ Done

As a user who loads a texture manually for a shader (or has an Assimp model loaded), I want
switching the active render target (primitive ↔ model, or model ↔ another model) to never leave a
texture bound in a GPU texture unit that belongs to a *different* render target than the one
currently being drawn.

**Why this priority**: Two independent stale-GL-state bugs were reported back-to-back: (a) after
loading an Assimp model, switching to a primitive (sphere/torus) kept showing the model's
texture, and (b), the reverse — after manually loading a texture onto a `sampler2D` uniform and
then importing a model, the model rendered with the manually loaded texture instead of its own
imported material textures.

**Independent Test 1 (model → primitive)**: Import a textured model, then switch to the "Torus" or
"Sphere" primitive; confirm it renders with no texture (flat/shader-default appearance) rather
than the model's texture.

**Independent Test 2 (manual texture → model)**: With a primitive selected, use "Load Image" to
assign a texture to a `sampler2D` uniform, then import a textured model (e.g. a glTF asset with a
base color texture); confirm the model renders with its own imported texture, not the manually
loaded one.

**Acceptance Scenarios**:

1. **Given** a `sampler2D` uniform whose current value is empty (no texture assigned for this
   render target), **When** the frame is rendered, **Then** the renderer explicitly unbinds
   (`glBindTexture(GL_TEXTURE_2D, 0)`) whatever texture is currently sitting in that uniform's
   texture unit before drawing, instead of leaving a previous render target's texture bound there
   and implicitly reused.
2. **Given** an Assimp model is imported, **When** it becomes the active render target, **Then**
   any texture the user had assigned by hand to a `sampler2D` uniform is cleared first, so the
   model always starts by showing only its own imported material textures (or nothing, for a slot
   the model's material doesn't populate).
3. **Given** the user switches from a model back to a primitive, **When** the primitive becomes
   the active render target, **Then** any texture belonging to the model is cleared — built-in
   primitives never inherit an Assimp-imported texture.

---

### User Story 3 - Built-in primitives share one texture set independent from the model's (Priority: P2) — ✅ Done

As a user manually assigning a texture to a `sampler2D` uniform while previewing a built-in
primitive, I want that texture to persist when I switch between *other* built-in primitives (e.g.
plane → sphere), and to come back exactly as I left it when I return from viewing an imported
model — while the model keeps rendering with its own textures the whole time, completely
independently.

**Why this priority**: Follows directly from User Story 2's fix: naively clearing all texture
uniforms on *every* render-target change also wiped the user's manually assigned texture when
just flipping between primitives, which the user explicitly did not want.

**Independent Test**: Load a texture manually while viewing "Plane", switch to "Sphere" (texture
must still show), switch to a loaded model (model's own texture must show, not the manual one),
then switch back to any primitive (the manually loaded texture must reappear exactly as before).

**Acceptance Scenarios**:

1. **Given** the render target changes from one built-in primitive to another (both non-model),
   **When** the switch happens, **Then** no texture uniform is cleared or altered — the same
   manually assigned texture set continues to apply.
2. **Given** the render target changes from "Primitive" to "Model" (either direction), **When**
   the switch happens, **Then** the outgoing primitive's texture assignments are captured first
   and the model's (or the primitives', when returning) texture assignments are restored/cleared
   as appropriate, so each of "the primitives as a group" and "the currently active model" keeps
   an independent texture set that survives round-trips.

---

### User Story 4 - Plane, cube, sphere, and torus no longer display textures upside-down (Priority: P1) — ✅ Done

As a user loading an image texture onto any built-in primitive, I want the texture to display
right-side-up, matching how it looks when opened in a normal image viewer — not flipped
vertically.

**Why this priority**: Reported twice in the same session: first for the plane and cube (after the
User Story 1 dice-mapping rewrite touched their UVs), then — once those two were patched
individually — for the torus, sphere, and (still) the cube, revealing that the real defect was a
single, project-wide vertical-flip mismatch between how textures are decoded (`stbi_image` with
`stbi_set_flip_vertically_on_load(1)`) and the "v=0 at top" convention every shape's UV generator
was authored against, not a per-shape defect.

**Independent Test**: Load a texture with an obvious "up" (e.g. text, or an asymmetric image) onto
each of plane, cube, sphere, cylinder, and torus in turn; confirm all five display the texture
right-side-up.

**Acceptance Scenarios**:

1. **Given** any built-in primitive's generated UVs, **When** they are uploaded to the GPU,
   **Then** a single, shape-agnostic vertical flip (`v = 1 - v`) is applied uniformly across every
   primitive's texcoords as the last step of `makeBuiltInPrimitives()`, rather than baking an
   ad-hoc flip into some shapes' individual UV formulas and not others.
2. **Given** a new built-in primitive is added in the future, **When** its `makeXxx()` function is
   authored using the conventional "v=0 at the top of the texture" mapping for its shape, **Then**
   it automatically renders right-side-up without needing its own special-cased flip, because the
   single flip step at the end of `makeBuiltInPrimitives()` covers it.

## Success Criteria *(mandatory)*

- **SC-001**: The cube displays a complete, undistorted, right-side-up copy of the assigned
  texture on each of its 6 faces.
- **SC-002**: The plane, sphere, cylinder, and torus each display their assigned texture
  right-side-up, using a mapping conventional for that shape (planar / equirectangular /
  cylindrical+disc-caps / toroidal respectively).
- **SC-003**: Switching the render target to an Assimp-imported model always shows that model's
  own imported textures (or blank, for unpopulated slots) — never a texture manually assigned
  under a same-named `sampler2D` uniform for a primitive or a previously active model.
- **SC-004**: Switching between built-in primitives preserves whatever texture the user manually
  assigned; switching to a model and back to a primitive restores that same manually assigned
  texture unchanged.
- **SC-005**: No regression to existing primitive rendering, model rendering (materials, PBR,
  transparency, animation, cameras), or the existing automated test suite
  (`shader_editor_tests`).
