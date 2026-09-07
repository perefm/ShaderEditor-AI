# UI Workspace Contract

## Purpose

Define the minimum user-facing workspace surfaces and interactions required for
the shader editor desktop application.

## Required Panels

### Shader Editor Panel

- Displays editable vertex shader source.
- Displays editable fragment shader source.
- Indicates dirty state for unsaved changes.
- Provides open and save actions for shader files.
- Provides a visible update action for recompiling the active shader pair.
- Supports `Ctrl+Enter` as a shader update shortcut while editing.

### Render View Panel

- Displays the active shader pair rendered on the selected primitive.
- Allows switching between plane, cube, torus, and at least two additional 3D
  primitives.
- Shows a visible render or compile failure state when preview is unavailable.

### Uniforms Panel

- Lists editable uniforms discovered for the active shader program.
- Provides controls appropriate to the uniform value shape where supported.
- Applies changed values to the active render preview during normal use.
- Surfaces unsupported or invalid uniforms without crashing the workspace.

### Status or Diagnostics Panel

- Shows file, compile, link, and runtime errors relevant to the current session.
- Exposes enough context for the user to know whether the last action succeeded.

### Shader Errors Panel

- Displays the current shader compile and link error messages in a dedicated
  dockable panel.
- Remains available after failed shader updates so users can correct the source.

## Docking Contract

- All major panels MUST be dockable within the main workspace.
- Docking or undocking a panel MUST not discard editor content or active preview
  state.
- The workspace MUST remain usable after layout changes.

## Interaction Contract

- Opening shader files MUST populate the editor panels for the active shader pair.
- Saving shader files MUST persist the current editor content for each shader
  type.
- Pressing `Ctrl+Enter` MUST trigger shader recompilation for the active shader
  pair.
- Clicking the update button MUST trigger shader recompilation for the active
  shader pair.
- Selecting a preview primitive MUST update the render view to that primitive.
- Changing a supported uniform value MUST update the render view without
  requiring the user to reopen the shader files.
- If compile or render fails, the application MUST keep the UI responsive and
  display a visible failure state in the dedicated shader errors panel.
