# Quickstart: Verifying Editor Menu Refinements, Phoenix Auto-Uniforms, and Assimp Model Import

This quickstart is the manual verification checklist referenced by the plan's
Constitution Check ("Verification"). Run after building `shader_editor`
(existing CMake/vcpkg build) on Windows.

## Prerequisites

- Build succeeds: `cmake --preset <existing-preset>` then build (see
  `CMakePresets.json` / existing README build instructions).
- `vcpkg.json` includes `assimp`; re-run vcpkg install/build after pulling
  this feature so the new dependency is available.
- A small sample rigged/animated/textured model file available locally for
  manual testing (e.g. a CC0 test asset such as Khronos's sample
  `CesiumMan.glb` or any small `.fbx`/`.gltf` with one diffuse texture and one
  animation clip), plus a Phoenix-style skinning shader that declares
  `texture_diffuse1`, `gBones`, `aBoneID`, `aBoneWeight`.

## 1. Save As (User Story 1)

1. Launch the app, open an existing example shader (File → Open Shader...).
2. Edit the source slightly.
3. File → **Save As**, choose a new file name in a different or same folder,
   confirm.
4. Verify: a new `.glsl` file exists on disk with the edited content; the
   editor title/diagnostics reflect the new path; the document is no longer
   marked dirty.
5. Press Ctrl+S ("Save shader"); verify it writes to the **new** path (check
   file timestamp), not the original file.
6. Repeat step 3 but press Cancel in the dialog; verify no new file is
   created and the original path/dirty state is unchanged.

## 2. Menu renames (User Story 2)

1. Open the File menu.
2. Verify items read exactly "Save shader" (Ctrl+S) and "Update Shader"
   (Ctrl+Enter).
3. Trigger each and confirm existing behavior (save to current path;
   recompile/apply to Render View) is unchanged.

## 3. Phoenix auto-uniforms (User Story 3)

1. Open/write a shader declaring `uniform float t; uniform float tend; uniform float beat;`
   and use them visibly (e.g. drive a color or position by `sin(t)`).
2. Update Shader; verify the Render View animates once playback is running.
3. Use Pause; verify the image freezes. Use Play; verify it resumes from the
   same point (not from 0).
4. Use Reset; verify the image returns to its `t=0` appearance.
5. Edit the "Section duration" (`tend`) field; verify (e.g. via a debug
   expression in the shader like coloring based on `tend`) that the shader
   receives the exact new value on the next frame, regardless of Play/Pause
   state.
6. Set `tend` to `1.0`, `0.0`, or a negative number; verify the displayed
   value is clamped to a value strictly greater than `1.0`.
7. Let playback reach `tend`; verify `t` resets to `0.0` and starts advancing
   again from the beginning.
8. While `t` is above a proposed new `tend`, reduce the `tend` field; verify
   `t` resets immediately to `0.0`.
9. Edit the BPM field to a positive value; verify `beat`-driven visuals pulse
   accordingly. Set BPM to 0 or a negative number; verify no NaN/flicker/crash
   and that `beat`-driven visuals hold steady (beat pinned at 0).
10. Open the Uniforms panel; verify `t`, `tend`, and `beat` (when declared by
   the shader) are shown as read-only/auto-managed, with no editable control,
   distinct from ordinary user uniforms.
11. Open/write a shader that does NOT declare `t`/`tend`/`beat` at all; verify
   it still compiles/runs with no warnings about their absence.

## 4. Assimp model import (User Story 4)

1. Use File → **Open Model...** (or equivalent new action) and select the
   sample rigged/textured/animated model.
2. Verify: the Render View now shows the model instead of the previous
   primitive, using the currently active shader.
3. With a shader declaring `uniform sampler2D texture_diffuse1;`, verify the
   model's diffuse texture appears correctly mapped. For `Fox.glb`, the image
   is embedded in the `.glb`; for external-texture models, verify the resolved
   image path is loaded.
4. With a Phoenix-style skinning shader declaring `uniform mat4 gBones[...]`
   and consuming `aBoneID`/`aBoneWeight`, verify the animation plays back
   using the same Play/Pause/Reset transport as User Story 3, and that
   Pause/Reset affect the model's animation pose exactly like they affect
   `t`.
5. Select a built-in primitive (e.g. "cube") from the existing primitive
   selector; verify the Render View switches back to the primitive.
6. Re-select the model as the render target (new `selectModel()`-style
   action); verify it reappears without needing to re-open the file (no
   re-import delay).
7. Open a different shader file (not a new model) while the model is
   selected as the render target; verify the model stays loaded and the new
   shader is applied to it.
8. Attempt to open an invalid/corrupt model file (or one referencing a
   missing texture); verify a diagnostic appears in the Diagnostics panel,
   the app does not crash, and the previously active render target (model or
   primitive) remains visible/unaffected.
9. Confirm the import of the sample model completes quickly enough that the
   UI does not appear frozen (SC-006) — no explicit timing threshold beyond
   "no perceptible freeze" for the sample-sized assets used in testing.
10. Open the Render panel's **Open Model...** button and verify it invokes the
    same native dialog and import path as File → Open Model....
11. For a model with multiple animation clips, choose each entry in the
    **Animation** selector and verify the selected clip changes. Choose
    **None** and verify the bind pose is displayed.

## 5. Engine uniforms and normalized beat

1. Open **Shader Help** and verify it documents `MVP`, `model`, `uCameraPos`,
   `t`, `tend`, `beat`, `Mat_*`, `texture_*`, and `gBones`, plus the Phoenix
   vertex attributes.
2. Declare `uniform mat4 model;` in a model shader. Verify it is uploaded
   automatically and does not appear as an editable row in **Uniforms**.
3. At 120 BPM, use a shader that visualizes `beat` and verify it follows
   `0.0 -> 0.5 -> 0.0` at `0.0 -> 0.25 -> 0.5` seconds. Confirm its value
   remains in `[0, 1)`; BPM `0` and negative BPM hold it at `0.0`.

## 6. Regression pass

- Run existing unit tests (`ctest` via the `tests` target) and confirm all
  previously passing tests (`test_example_shaders`, `test_preview_camera`,
  `test_render_session`, `test_shader_file_service`,
  `test_uniform_introspection`, `test_workspace_models`) still pass, plus the
  new tests added for this feature
  (`test_playback_clock`, `test_assimp_model_loader`, and the extended
  cases in `test_shader_file_service`, `test_uniform_introspection`,
  `test_workspace_models`). The final implementation passes the complete
  Debug CTest suite.

## Closure

This quickstart was updated and completed on 2026-09-07 for the closed spec.
