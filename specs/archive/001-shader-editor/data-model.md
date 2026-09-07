# Data Model: Multiplatform Shader Editor

## ShaderPairDocument

**Purpose**: Represents the active editable shader pair in the workspace.

**Fields**:
- `vertexPath`: optional absolute or project-relative path to the vertex shader
- `fragmentPath`: optional absolute or project-relative path to the fragment shader
- `vertexSource`: current vertex shader text
- `fragmentSource`: current fragment shader text
- `isDirty`: whether either editor buffer contains unsaved changes
- `lastLoadedAt`: timestamp of the last successful load
- `lastSavedAt`: timestamp of the last successful save

**Validation Rules**:
- Both source buffers must remain independently editable.
- Save actions must not swap or merge vertex and fragment content.
- Dirty state must become `true` after local edits and `false` after successful save.

**State Transitions**:
- `empty` -> `loaded`
- `loaded` -> `dirty`
- `dirty` -> `saved`
- `dirty` -> `discarded`
- any state -> `error`

## UniformDefinition

**Purpose**: Describes one editable runtime shader input exposed to the user.

**Fields**:
- `name`: shader uniform identifier
- `kind`: scalar, integer, boolean, vector, matrix, sampler, or color-like value
- `componentCount`: number of editable components when applicable
- `defaultValue`: initial runtime value
- `currentValue`: active value sent to the render preview
- `editable`: whether the control is exposed in the current panel
- `validationRule`: optional range or formatting rule

**Validation Rules**:
- Uniform names must remain unique within the active shader program.
- Current value must satisfy the expected shape for its kind.
- Unsupported uniform kinds must surface as non-editable or explicitly unsupported.

**State Transitions**:
- `discovered` -> `editable`
- `editable` -> `applied`
- `editable` -> `invalid`

## PreviewPrimitive

**Purpose**: Represents a built-in geometry option in the render preview.

**Fields**:
- `id`: stable primitive identifier
- `label`: user-visible name
- `meshSource`: built-in mesh definition or generator
- `defaultTransform`: default position, rotation, and scale
- `availability`: whether the primitive is ready for preview

**Validation Rules**:
- Required primitives include plane, cube, and torus.
- At least two additional 3D primitives must be present.

**State Transitions**:
- `registered` -> `ready`
- `ready` -> `selected`

## RenderSession

**Purpose**: Captures the current preview state driven by the active shader pair.

**Fields**:
- `selectedPrimitiveId`: current preview mesh
- `programStatus`: uncompiled, compiled, linked, or failed
- `uniformValues`: applied runtime values keyed by uniform name
- `frameStatus`: rendering, idle, or error
- `errorMessage`: latest visible compile or render failure summary

**Validation Rules**:
- Render session must apply only the active shader pair.
- Model switches and uniform updates must preserve a valid render loop.
- Failures must update `errorMessage` without crashing the app.

**State Transitions**:
- `idle` -> `compiling`
- `compiling` -> `ready`
- `compiling` -> `error`
- `ready` -> `rendering`
- `rendering` -> `error`

## WorkspaceLayout

**Purpose**: Stores the arrangement of dockable windows for the active session.

**Fields**:
- `dockNodes`: serialized docking arrangement
- `openPanels`: visible panel identifiers
- `focusedPanel`: currently focused panel
- `layoutPreset`: default or user-customized arrangement

**Validation Rules**:
- Editor and render panels must remain dockable.
- Layout changes must not destroy core panel content.

**State Transitions**:
- `default` -> `customized`
- `customized` -> `restored`
