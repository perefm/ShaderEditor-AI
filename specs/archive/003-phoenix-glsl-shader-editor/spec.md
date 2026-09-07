# Feature Specification: Phoenix Single-File GLSL Shader Editor

**Feature Branch**: `003-phoenix-glsl-shader-editor`
**Created**: 2026-09-07
**Status**: Complete
**Graphics API**: OpenGL 4.6 core profile / GLSL 4.60
**Input**: User description: "Adopt Phoenix single-file GLSL shaders with unified editing, source line diagnostics, sampler2D image uniforms, and example shaders"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Open and save a Phoenix shader file (Priority: P1)

As a shader author, I want to open one `.glsl` file containing both shader
stages, edit it as one document, and save it back without splitting it into
separate vertex and fragment files.

**Why this priority**: The single-file format is the foundation for all other
requirements and aligns ShaderEditor with the Phoenix engine workflow.

**Independent Test**: Open a `.glsl` file containing `#type vertex` and
`#type fragment` sections, confirm both stages are loaded, edit text in either
section, save, and verify the saved file preserves both markers and source.

**Acceptance Scenarios**:

1. **Given** a valid Phoenix `.glsl` file with `#type vertex` followed by
   `#type fragment`, **When** the user opens it, **Then** the complete source
   appears in one editor and both stages are available to the renderer.
2. **Given** an edited single-file shader, **When** the user saves it, **Then**
   one `.glsl` file is written and no separate `.vert` or `.frag` file is
   required or generated.
3. **Given** a file with an unsupported extension or missing stage marker,
   **When** the user tries to open it, **Then** the operation is rejected with
   a diagnostic that identifies the expected Phoenix format.
4. **Given** a valid file whose sections contain comments, blank lines,
   `#version`, uniforms, and normal GLSL code, **When** it is parsed, **Then**
   all text is retained exactly in the unified editor and stage extraction does
   not drop meaningful lines.

---

### User Story 2 - Edit both stages in one numbered editor (Priority: P1)

As a shader author, I want the vertex and fragment code displayed as one
continuous text document with visible line numbers, so I can navigate and
reason about the exact source sent to Phoenix/OpenGL.

**Why this priority**: A unified editor is the requested editing model and is
necessary for diagnostics to use the same line numbering as the source file.

**Independent Test**: Open a multi-stage `.glsl` file, confirm one editor is
shown with continuous line numbers across the `#type` boundary, edit lines in
both sections, and verify dirty-state/save behavior works for the one document.

**Acceptance Scenarios**:

1. **Given** a file with 10 vertex lines before the fragment section, **When**
   the fragment section is displayed, **Then** its first source line uses the
   actual document line number rather than restarting at line 1.
2. **Given** the unified editor is open, **When** the user edits either stage,
   **Then** the document becomes dirty and the same source is used by the next
   shader update.
3. **Given** the editor is resized or scrolled, **When** the user navigates,
   **Then** line numbers remain aligned with the corresponding source lines.
4. **Given** an unsaved unified document, **When** the user loads another file
   or exits, **Then** the existing unsaved-changes flow warns before discarding
   the single source document.

---

### User Story 3 - Report shader errors using real document lines and stage (Priority: P1)

As a shader author, I want shader compilation errors to identify whether they
are in the vertex or fragment stage and point to the actual line in the unified
editor, including all preceding lines from the other stage.

**Why this priority**: Accurate source locations are essential for fixing
shaders quickly and are a core benefit of using Phoenix's section format.

**Independent Test**: Create a file whose fragment shader has an error on its
second fragment-source line after 10 vertex lines. Compile it and verify the
error identifies `Fragment` and unified document line 12 (with the exact
marker/blank-line convention defined by the parser), then repeat for Vertex.

**Acceptance Scenarios**:

1. **Given** a compile error on fragment source line 2 after 10 vertex source
   lines, **When** compilation fails, **Then** the shader error reports stage
   `Fragment` and the corresponding real unified-editor line (12 when the
   section markers are excluded from the count, or the parser's documented
   source-line mapping when markers are included).
2. **Given** a compile error in the vertex section, **When** compilation fails,
   **Then** the shader error reports stage `Vertex` and the actual unified-file
   line.
3. **Given** multiple errors, **When** diagnostics are shown, **Then** each
   message retains stage, line, and compiler text and can be correlated to the
   numbered editor.
4. **Given** an error line, **When** the user selects or activates it in the
   shader-errors panel, **Then** the editor scrolls to or highlights the
   corresponding unified source line when the UI supports navigation.
5. **Given** a failed compile, **When** the error is displayed, **Then** the
   last successfully compiled render program and uniform state remain usable
   until a later compile succeeds.

---

### User Story 4 - Edit sampler2D uniforms with image files (Priority: P1)

As a shader author, I want `sampler2D` uniforms to appear in the Uniforms
panel and let me choose an image file, so texture-based shaders can be previewed
without manually wiring OpenGL texture handles.

**Why this priority**: Texture uniforms are required for common Phoenix shaders
and are explicitly part of the requested feature.

**Independent Test**: Open an example shader containing `uniform sampler2D
u_texture;`, choose a supported image in the Uniforms panel, and verify the
preview uses the selected image; then compile a shader with a different
sampler2D name and repeat.

**Acceptance Scenarios**:

1. **Given** a successful shader compile containing `uniform sampler2D
   u_texture;`, **When** the Uniforms panel is displayed, **Then** it shows an
   image selector for `u_texture` rather than an unsupported/read-only control.
2. **Given** a sampler2D selector, **When** the user chooses a supported image
   (`.png`, `.jpg`, `.jpeg`, `.bmp`, or `.tga`), **Then** the image is decoded
   with `stb_image`, uploaded as an OpenGL texture, and bound to the uniform
   for preview rendering.
3. **Given** a selected image and a successful recompile that keeps the same
   sampler2D uniform name, **When** the panel refreshes, **Then** the selected
   image remains selected and its texture remains usable.
4. **Given** an image cannot be read, decoded, uploaded, or is unsupported,
   **When** the user selects it, **Then** the panel reports a visible error and
   keeps the previous valid texture selection.
5. **Given** a sampler2D uniform is removed or changes to another type after a
   successful compile, **When** the panel refreshes, **Then** its old image
   selection is removed and is not applied to the incompatible uniform.

---

### User Story 5 - Use bundled Phoenix-format examples (Priority: P2)

As a new user, I want several ready-to-open `.glsl` shader examples in
`assets/shaders`, including at least one texture shader, so I can validate the
editor and preview immediately.

**Why this priority**: Examples make the new file format discoverable and
provide repeatable manual validation for stages, errors, uniforms, and textures.

**Independent Test**: Start the application, load each bundled example, compile
it, and confirm its preview and declared uniforms work without external shader
files.

**Acceptance Scenarios**:

1. **Given** the installed application, **When** the user opens the example
   shader list, **Then** several `.glsl` files are available under
   `assets/shaders`.
2. **Given** the texture example, **When** it is loaded, **Then** its bundled
   image asset is available and the sampler2D control can select it.
3. **Given** any bundled example, **When** it is compiled, **Then** it uses the
   same single-file parser and unified editor as user-provided shaders.

---

### Edge Cases

- A `.glsl` file has duplicate `#type vertex` or `#type fragment` markers.
  Loading must reject it with a clear format diagnostic rather than guessing.
- The markers have different casing or extra trailing text. The accepted
  syntax and case sensitivity must be documented and enforced consistently.
- A file has a fragment section before a vertex section. The parser may accept
  either order only if line mapping and rendering remain deterministic; the
  canonical Phoenix order is vertex followed by fragment.
- A marker appears inside a GLSL comment or string. The parser must not treat
  commented text as a stage delimiter if the Phoenix format rules exclude it.
- A shader has no uniforms, has only a sampler2D, or declares the same uniform
  in both stages. The panel must show a deterministic single editable entry
  per active uniform name and stage/link validation must remain explicit.
- A sampler2D image has an alpha channel, grayscale format, unusual dimensions,
  or is very large. The loader must normalize supported channel data safely and
  surface resource failures without crashing or blocking the UI indefinitely.
- A failed compile reports a vendor-specific log format or no parseable line.
  The diagnostic must preserve the raw compiler text, identify the known stage
  when possible, and avoid inventing an incorrect line number.
- The unified editor includes CRLF or LF line endings. Line calculations must
  use logical lines consistently and preserve the file's text on save where
  practical.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST use an OpenGL 4.6 core-profile context for the application preview and shader compilation; context creation MUST request major version 4 and minor version 6 and fail clearly when unavailable.
- **FR-002**: All bundled and user-facing shader stages MUST target GLSL 4.60 using `#version 460 core`; no new or migrated example shader may continue targeting GLSL 3.30.
- **FR-003**: System MUST treat `.glsl` as the primary shader file format for
  opening and saving a complete vertex/fragment shader pair.
- **FR-004**: System MUST parse the Phoenix stage markers `#type vertex` and
  `#type fragment` and extract both stages from one source document.
- **FR-005**: System MUST reject missing, duplicate, malformed, or otherwise
  ambiguous stage markers with a visible diagnostic; it MUST NOT silently
  compile an incomplete or guessed stage pair.
- **FR-006**: System MUST store and edit the complete Phoenix source as one
  document and MUST stop requiring separate vertex and fragment source buffers
  or separate shader file paths for the normal workflow.
- **FR-007**: System MUST present one editor containing the complete source,
  including stage markers, with visible one-based line numbers aligned to every
  displayed line.
- **FR-008**: System MUST maintain a source map from each extracted stage line
  to its real unified-document line.
- **FR-009**: System MUST translate compiler diagnostics into stage, unified
  document line, and message fields whenever the compiler provides a source
  line; the displayed line MUST include all preceding source lines and follow
  the documented marker/blank-line mapping.
- **FR-010**: System MUST identify errors as `Vertex` or `Fragment` in shader
  diagnostics whenever the failing stage is known.
- **FR-011**: System MUST preserve raw compiler output when line/stage parsing
  is unavailable and MUST NOT report a fabricated source location.
- **FR-012**: System MUST refresh uniforms only after a successful compile/link
  of the parsed stage pair and MUST retain the last successful program and
  uniform state after a failed compile.
- **FR-013**: System MUST discover `sampler2D` declarations and expose them as
  editable texture uniforms in the Uniforms panel.
- **FR-014**: System MUST use `stb_image` to decode supported image files and
  report image loading errors without terminating the application.
- **FR-015**: System MUST upload selected images to OpenGL textures, bind them
  to the correct sampler2D uniform unit, and release/reuse texture resources
  without leaking them when selections or shader programs change.
- **FR-016**: System MUST preserve a sampler2D's selected image across a
  successful recompile only when its uniform name and sampler2D type remain
  unchanged.
- **FR-017**: System MUST allow users to choose images from the Uniforms panel
  using the existing desktop file-dialog conventions and show the selected
  path or filename.
- **FR-018**: System MUST add multiple valid `.glsl` examples under
  `assets/shaders`, including examples for basic rendering, uniforms, compile
  diagnostics, and sampler2D image preview.
- **FR-019**: System MUST package example shader and image assets beside the
  executable using the existing runtime-assets copy behavior.
- **FR-020**: System MUST preserve existing primitive selection, camera
  interaction, docking, diagnostics, and Debug/Release build workflows unless
  explicitly changed by this spec.
- **FR-021**: System MUST keep editor, parsing, image selection, and preview
  interactions responsive during normal use and MUST surface failures through
  existing diagnostics/error panels rather than silently ignoring them.
- **FR-022**: The preview engine MUST provide Phoenix-compatible implicit
  uniforms named `MVP` (`mat4`) and `uCameraPos` (`vec3`). These uniforms MUST
  be updated for every rendered frame, MUST be excluded from the editable
  Uniforms panel, and MUST be documented in Shader Help.

### Key Entities *(include if feature involves data)*

- **PhoenixShaderDocument**: One `.glsl` source document containing ordered
  `#type vertex` and `#type fragment` sections, its file path, dirty state, and
  mappings between extracted stage lines and unified editor lines.
- **ShaderStageSource**: A parsed stage (`Vertex` or `Fragment`) containing its
  source text, marker position, source-line offset, and compiler-diagnostic
  mapping information.
- **ShaderDiagnostic**: A structured error with stage (when known), unified
  document line (when known), column (when known), raw compiler message, and
  severity.
- **TextureUniformState**: The selected image path, decoded/uploaded texture
  identity, sampler unit, and load status associated with a `sampler2D`
  uniform.
- **UniformDefinition**: Existing uniform metadata extended to represent
  `sampler2D`, its selected texture value, and compatibility rules for refresh.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: The application creates an OpenGL 4.6 core-profile context on every supported target, or reports a clear unsupported-version diagnostic.
- **SC-002**: 100% of valid Phoenix `.glsl` examples load into one editor and
  compile without requiring separate stage files.
- **SC-003**: For a test fixture with 10 vertex source lines and a fragment
  error on fragment source line 2, the displayed diagnostic identifies
  `Fragment` and the exact unified-editor line defined by the source map (line
  12 for the specified no-marker-offset example).
- **SC-004**: 100% of compiler diagnostics with parseable stage/line data show
  the correct stage and unified line; unparseable logs retain raw text without
  false coordinates.
- **SC-005**: A user can load an image into a sampler2D uniform and see the
  selected texture applied in the preview within the next rendered frame.
- **SC-006**: Failed image selections and failed shader compiles leave the last
  valid preview, shader program, and texture selection usable in 100% of
  tested failure cases.
- **SC-007**: At least four bundled `.glsl` examples, including one sampler2D
  example with an image asset, are available and compile successfully in both
  Debug and Release builds.
- **SC-008**: The normal open/edit/compile/save workflow uses one source editor
  and one `.glsl` file with no manual stage splitting.

## Assumptions

- Phoenix's canonical stage syntax is exactly `#type vertex` and `#type
  fragment`, with vertex before fragment in the bundled examples.
- OpenGL 4.6 core profile and GLSL 4.60 are mandatory targets for the context, compiler, and every bundled shader; existing primitive rendering remains the baseline;
  this feature adds file parsing, diagnostics mapping, and textures rather than
  changing the rendering API wholesale.
- `stb_image` is introduced as a source/header dependency already suitable for
  this C++20/vcpkg build; no network service is required at runtime.
- Image paths are local filesystem paths and are not persisted beyond the
  current in-memory uniform state unless existing workspace persistence is
  explicitly extended by a later feature.
- The existing Windows native file-dialog path is reused first; equivalent
  platform behavior remains subject to the project's current desktop support.
- Phoenix runtime expressions and non-GLSL template syntax are out of scope;
  ShaderEditor supports the shader-file stage markers and GLSL declarations
  needed for its own OpenGL preview.