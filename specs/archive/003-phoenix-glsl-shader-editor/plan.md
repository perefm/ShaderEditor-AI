# Implementation Plan: Phoenix Single-File GLSL Shader Editor

**Branch**: `main` | **Date**: 2026-09-07 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/archive/003-phoenix-glsl-shader-editor/spec.md`

## Summary

Migrate ShaderEditor from separate `.vert`/`.frag` files to Phoenix-compatible single-file `.glsl` documents containing `#type vertex` and `#type fragment` sections. The implementation will keep the complete source as one editable, line-numbered document, parse stage-specific source maps for accurate diagnostics, require an OpenGL 4.6 core context with GLSL 4.60 shaders, and add image-backed `sampler2D` uniforms using `stb_image`. Bundled examples and runtime asset copying will be updated to exercise the new workflow.

## Technical Context

**Language/Version**: C++20  
**Graphics API**: OpenGL 4.6 core profile, GLSL 4.60  
**Primary Dependencies**: GLFW, glad, Dear ImGui, GLM, stb_image, Catch2  
**Storage**: Local `.glsl` files and image assets; selected texture state remains in memory  
**Testing**: Catch2 unit tests for parsing/source maps/uniform state plus Debug/Release builds and manual OpenGL/ImGui smoke validation  
**Target Platform**: Existing desktop targets with an OpenGL 4.6-capable driver; unsupported contexts must fail with a visible diagnostic  
**Project Type**: C++ desktop application  
**Performance Goals**: Preserve the existing responsive render loop; ordinary shader parsing and image selection must not block UI interaction beyond the current compile/upload operation  
**Constraints**: Preserve docking, camera, primitive, diagnostics, and last-valid-preview behavior; do not silently guess malformed Phoenix sections; keep compiler line mapping deterministic for LF and CRLF files  
**Scale/Scope**: One active Phoenix shader document, two shader stages, existing primitive preview, nine existing uniform value types plus `sampler2D`, and at least four bundled examples

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- Platform compatibility: Pass with an explicit capability gate. GLFW must request an OpenGL 4.6 core context; initialization reports a clear unsupported-version error rather than falling back to 3.3. Platform-specific native dialogs remain behind the existing abstraction.
- Verification: Pass. Add parser and diagnostic mapping tests, sampler2D/state tests that do not require a live GPU where possible, and retain Debug/Release CMake/CTest plus manual context, image, and render validation.
- UI consistency: Pass. Keep the existing dockable Shader Editor, Uniforms, Diagnostics, and Shader Errors panels. Replace the two source controls with one line-numbered editor and extend existing uniform controls with an image picker.
- OpenGL/runtime impact: Pass with safeguards. Compile stages transactionally, retain the last valid program on failure, validate texture decoding/upload failures, and release/reuse GL texture objects deterministically.

## Phase 0: Research and implementation decisions

1. Confirm Phoenix section semantics from the reference format: exact markers are `#type vertex` and `#type fragment`, with one section per stage and vertex-before-fragment as the canonical form.
2. Decide the source-map convention explicitly: preserve marker lines in the editor, map each extracted GLSL line back to its one-based unified-document line, and define how marker and blank lines affect compiler offsets.
3. Inventory the current document/file APIs and replace paired paths/sources with one `.glsl` path plus a parsed representation that can be regenerated after every edit.
4. Confirm the OpenGL 4.6 request and runtime version check in `WindowContext`; identify the exact failure diagnostic and whether the requested vcpkg/glad configuration exposes the required symbols.
5. Confirm the available `stb_image` package/header integration and choose ownership boundaries for decoded pixels and GL texture objects.
6. Inspect current shader compiler error formats and define parsers for stage-local line numbers, preserving raw logs when parsing is not possible.
7. Define sampler2D value compatibility and lifetime rules: same name plus sampler type preserves the selected image; removed/type-changed uniforms release or detach prior texture state.

## Phase 1: Design and implementation

### Phoenix document and parser

- Replace the paired `ShaderPairDocument` fields with a single source string/path and parsed `ShaderStageSource` metadata, retaining dirty/load/save timestamps.
- Add a Phoenix shader parser service that validates extension, marker count/order, non-empty stages, and duplicate/malformed markers.
- Store stage source ranges and a one-based mapping from stage-local lines to unified editor lines; support LF and CRLF without changing displayed line numbers.
- Make `ShaderFileService` open/save one `.glsl` file and remove the normal workflow's dependency on separate `.vert`/`.frag` paths.
- Update all open/load/save dialogs, example loading, and editor-state mutation paths to use the unified source.

### OpenGL 4.6 and shader compilation

- Change `WindowContext` to request `GLFW_CONTEXT_VERSION_MAJOR = 4`, `GLFW_CONTEXT_VERSION_MINOR = 6`, and the core profile.
- Verify the created context version after glad initialization and report an actionable diagnostic if it is below 4.6 or context creation fails.
- Update ImGui's OpenGL backend initialization and shader source assumptions to GLSL 4.60-compatible strings where required.
- Compile the parsed vertex and fragment sources with `#version 460 core` examples and preserve the existing transactional last-valid render session behavior.
- Extend shader compile results and diagnostics with stage, stage-local line, mapped unified line, column when available, and raw compiler text.
- Ensure failed parsing/compilation/linking never replaces the last valid program, uniforms, or texture bindings.

### Unified line-numbered editor and diagnostics UX

- Replace the two multiline source widgets with one editor buffer containing the Phoenix markers and complete source.
- Implement a synchronized line-number gutter with one-based numbers aligned during scrolling and editing; keep dirty-state updates tied to this single buffer.
- Extend `Shader Errors` entries to show `Vertex` or `Fragment`, unified line, optional column, and message.
- Add navigation/highlighting hooks from an error entry to the corresponding unified editor line where supported by the current ImGui editor implementation.
- Update labels/help text to explain the Phoenix markers and actual-file line numbering.

### Uniforms and sampler2D textures

- Extend `UniformDefinition`/`UniformValue` and `UniformState` with a sampler2D texture selection/value representation and compatible refresh rules.
- Discover `sampler2D` declarations in both parsed stages, deduplicating linked uniforms deterministically and rejecting incompatible duplicate declarations.
- Add a texture asset service using `stb_image` for supported formats, channel normalization, validation, and error reporting.
- Add GL texture ownership/binding to the preview/render service with stable sampler units, filtering/wrapping defaults, replacement cleanup, and shutdown cleanup.
- Add an image chooser control to the Uniforms panel using existing native dialog conventions; display selected filename/path and preserve the previous valid selection after failures.
- Keep image selection across successful recompiles only when the sampler name and type remain compatible, matching the existing value-preservation behavior for scalar/vector/matrix uniforms.

### Example assets and build integration

- Replace `.vert`/`.frag` examples in `assets/shaders` with at least four `.glsl` files using `#version 460 core`: basic primitive, editable uniforms, diagnostic/error fixture, and textured sampler2D preview.
- Add at least one small image asset for the sampler2D example and ensure its licensing/source is appropriate for repository inclusion.
- Update CMake runtime asset copying to include `.glsl` and image assets beside the executable in Debug and Release.
- Update the example shader discovery/open flow and README/quickstart documentation to describe the Phoenix format and OpenGL 4.6 prerequisite.

### Tests and validation

- Add parser tests for valid sections, missing/duplicate/malformed markers, order, comments, blank lines, LF/CRLF, and stage-local-to-unified line mapping.
- Add diagnostic tests for vertex/fragment stage labels, compiler line translation, marker offsets, unparseable logs, and multiple errors.
- Add uniform tests for sampler discovery, duplicate handling, image-selection preservation, type changes, and failed image loads.
- Add non-GPU tests for source/document transitions and example asset presence; use manual smoke validation for actual context creation, texture upload, render output, editor gutter alignment, and error navigation.
- Run Debug and Release builds/tests and verify the application reports a clear unsupported OpenGL version instead of silently falling back.

## Project Structure

### Documentation (this feature)

```text
specs/archive/003-phoenix-glsl-shader-editor/
|-- spec.md
|-- plan.md
|-- research.md
|-- data-model.md
|-- quickstart.md
|-- contracts/
|   `-- phoenix-glsl.md
`-- tasks.md
```

### Source Code (repository root)

```text
src/editor/ShaderPairDocument.h
src/editor/ShaderEditorState.h
src/editor/ShaderEditorState.cpp
src/services/files/ShaderFileService.h
src/services/files/ShaderFileService.cpp
src/services/shaders/PhoenixShaderParser.h
src/services/shaders/PhoenixShaderParser.cpp
src/services/images/TextureAssetService.h
src/services/images/TextureAssetService.cpp
src/app/platform/WindowContext.h
src/app/platform/WindowContext.cpp
src/app/workspace/WorkspaceController.h
src/app/workspace/WorkspaceController.cpp
src/app/workspace/UniformState.h
src/app/workspace/UniformState.cpp
src/rendering/shaders/ShaderProgramService.h
src/rendering/shaders/ShaderProgramService.cpp
src/rendering/shaders/UniformDefinition.h
src/rendering/shaders/UniformIntrospectionService.cpp
src/rendering/opengl/PreviewRenderer.h
src/rendering/opengl/PreviewRenderer.cpp
src/ui/panels/ShaderEditorPanel.h
src/ui/panels/ShaderEditorPanel.cpp
src/ui/panels/ShaderErrorsPanel.h
src/ui/panels/ShaderErrorsPanel.cpp
src/ui/panels/UniformsPanel.h
src/ui/panels/UniformsPanel.cpp
src/app/application/Application.cpp
assets/shaders/*.glsl
assets/textures/*
tests/unit/test_phoenix_shader_parser.cpp
tests/unit/test_shader_diagnostics.cpp
tests/unit/test_texture_uniforms.cpp
tests/unit/test_workspace_models.cpp
```

**Structure Decision**: Keep parsing and source mapping in a dedicated service, keep document/edit state in the editor layer, keep compile/transaction ordering in `WorkspaceController`, and keep OpenGL texture ownership in the rendering/image service boundary. The UI remains a consumer of unified document and structured diagnostics rather than parsing compiler strings itself.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Dedicated Phoenix parser and source-map model | One file must produce two compilable stages while preserving unified editor lines and exact diagnostics | Splitting strings ad hoc in UI/compiler code would duplicate rules and make line errors unreliable |
| Dedicated texture asset/GL ownership path | `sampler2D` requires decoded pixels, GL lifetime, sampler units, and failure-safe replacement | Passing raw filenames through `UniformValue` cannot guarantee upload, cleanup, or render correctness |