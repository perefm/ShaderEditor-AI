# Implementation Plan: Mega Shader de Materiales Assimp (PBR + Bones + 1 Luz)

**Branch**: `008-mega-shader-pbr-lighting` | **Date**: 2026-09-09 | **Spec**: [spec.md](/specs/008-mega-shader-pbr-lighting/spec.md)
**Input**: Feature specification from `/specs/008-mega-shader-pbr-lighting/spec.md`

## Summary

Consolidar los 7 shaders parciales de materiales Assimp (`bone_animation*.glsl`,
`bump_mapping.glsl`, `material_pixel_lighting.glsl`, `pbr_animation*.glsl`) en un único
**mega shader** (`assets/shaders/mega_material.glsl`) que soporta, en una sola pasada de render:
skinning por bones (con fallback a pose sin deformar), una luz puntual calculada por píxel, y
todos los mapas de material que el proyecto va a soportar de Assimp (base color, normal,
metalness/roughness, emissive, specular dedicado, height/displacement), seleccionando
automáticamente entre un camino PBR (Cook-Torrance) y un camino Blinn-Phong clásico según qué
datos aporte el material, con fallback a colores de material cuando no hay ninguna textura. Se
añaden dos shaders de estilo adicionales que reutilizan el mismo pipeline de datos
(`assets/shaders/toon_material.glsl`, `assets/shaders/rim_lighting_material.glsl`). Se extiende
`ModelMaterial`/`AssimpModelLoader` para cargar specular map y height/displacement map. Se
reorganiza y documenta el catálogo completo de shaders (`assets/shaders/README.md`). Ambient
occlusion y Bokeh/Depth-of-Field quedan explícitamente fuera de alcance (requieren pasadas de
render adicionales que la arquitectura actual de `PreviewRenderer` no soporta).

## Technical Context

**Language/Version**: C++20 (motor) + GLSL 460 core (shaders)
**Primary Dependencies**: OpenGL 4.6 core, GLFW, Dear ImGui, GLM, glad, Assimp (carga de modelos)
**Storage**: Archivos en disco (`assets/shaders/*.glsl`, modelos importados vía Assimp); sin base
de datos.
**Testing**: Catch2 (`tests/unit/*.cpp`, ejecutado vía CTest/`shader_editor_tests`).
**Target Platform**: Desktop multiplataforma (Windows es la plataforma de desarrollo activa;
build basado en CMake + vcpkg).
**Project Type**: Aplicación de escritorio (editor + motor de render OpenGL en un único
ejecutable).
**Performance Goals**: Mantener 60 FPS en el preview con el modelo de prueba (Fox.glb, coche PBR
de la spec 007) y el mega shader activo; sin regresión de FPS perceptible frente a los shaders
parciales actuales.
**Constraints**: Una sola pasada de render (sin depth-prepass, G-Buffer ni MRT); número de
texture units por material dentro de límites razonables (<16, típico mínimo garantizado por
OpenGL 4.6); no romper el ciclo de vida OpenGL (compilación de shaders, framebuffer, bucle de
frame) ni bloquear la UI.
**Scale/Scope**: ~10 archivos de shader afectados (7 legacy a deprecar/consolidar + 3 nuevos:
mega, toon, rim), 1 struct de datos (`ModelMaterial`) extendida con 2 mapas nuevos, 1 loader
(`AssimpModelLoader`) extendido, 1 método de bind de uniforms (`PreviewRenderer::bindMeshMaterial`)
extendido.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

- **Platform compatibility**: Sin cambios de plataforma. El mega shader usa GLSL 460 core, igual
  que todos los shaders existentes; no introduce extensiones específicas de un fabricante ni de
  una plataforma. Se valida en Windows (plataforma de desarrollo activa) mediante build y
  ejecución manual del preview.
- **Verification**: Se añaden/actualizan pruebas unitarias Catch2: (a) `AssimpModelLoader` carga
  correctamente los nuevos mapas (specular, height) cuando existen en el modelo de prueba; (b) los
  3 shaders nuevos (`mega_material.glsl`, `toon_material.glsl`, `rim_lighting_material.glsl`) se
  cargan y parsean vía `ShaderFileService` sin error (igual que `test_example_shaders.cpp` ya hace
  para `basic.glsl`). Verificación manual: cargar el modelo Fox (con esqueleto+textura) y un
  modelo/objeto sin texturas con el mega shader y comprobar visualmente ambos casos, más el modelo
  PBR con vidrio (spec 007) para no regresionar transmission/opacity.
- **UI consistency**: No se cambia el flujo de selección de shader existente (File > Open
  shader..., panel Render). Los 3 shaders nuevos se abren igual que cualquier otro `.glsl` del
  proyecto; no se añade un selector nuevo de UI (fuera de alcance según Assumptions de la spec:
  el mecanismo de selección de shader por modelo ya existente —abrir el archivo `.glsl`
  correspondiente— se reutiliza sin cambios).
- **OpenGL/runtime impact**: El mega shader compila y enlaza como cualquier otro programa GLSL vía
  el pipeline `ShaderProgramService` existente; no cambia el ciclo de vida de compilación/enlace.
  No se toca `PreviewRenderer::ensureFramebuffer` (se preserva la arquitectura de una sola pasada,
  un único framebuffer, sin depth-as-texture). Se valida que la compilación con el máximo de mapas
  soportados no introduce fallos de enlace (uniforms/samplers duplicados) ni cuelgues.

## Project Structure

### Documentation (this feature)

```text
specs/008-mega-shader-pbr-lighting/
├── plan.md              # This file
├── research.md          # Phase 0 output
├── data-model.md         # Phase 1 output
├── quickstart.md         # Phase 1 output
└── contracts/
    └── shader-catalog.md # Phase 1 output: catálogo documentado de todos los shaders
```

### Source Code (repository root)

```text
assets/
└── shaders/
    ├── README.md                        # NUEVO: catálogo/organización de todos los shaders
    ├── mega_material.glsl               # NUEVO: mega shader realista (bones+PBR/clásico+mapas)
    ├── toon_material.glsl               # NUEVO: variante estilizada (cel shading)
    ├── rim_lighting_material.glsl       # NUEVO: variante estilizada (realce de silueta)
    ├── basic.glsl                       # sin cambios (categoría: aprendizaje/plantilla)
    ├── textured.glsl                    # sin cambios (categoría: aprendizaje/plantilla)
    ├── pixel_lighting.glsl              # sin cambios (categoría: aprendizaje/plantilla)
    ├── diagnostics.glsl                 # sin cambios (categoría: aprendizaje/plantilla)
    ├── uniforms.glsl                    # sin cambios (categoría: aprendizaje/plantilla)
    └── legacy/                          # NUEVO subdirectorio: shaders Assimp parciales previos
        ├── bone_animation.glsl
        ├── bone_animation_bump_mapping.glsl
        ├── bone_animation_material_only.glsl
        ├── bump_mapping.glsl
        ├── material_pixel_lighting.glsl
        ├── pbr_animation.glsl
        └── pbr_animation_artistic.glsl

src/
├── rendering/
│   ├── models/
│   │   ├── ModelDocument.h              # ModelMaterial: + hasSpecularMap/hasHeightMap, etc.
│   │   └── AssimpModelLoader.cpp        # + carga aiTextureType_SPECULAR/HEIGHT/DISPLACEMENT
│   └── opengl/
│       └── PreviewRenderer.cpp          # bindMeshMaterial: + nuevos uniforms hasXxx/samplers
└── app/application/
    └── Application.cpp                  # sin cambios funcionales (mismo flujo Open shader...)

tests/unit/
├── test_assimp_model_loader.cpp         # + casos para specular/height map
└── test_example_shaders.cpp             # + casos para los 3 shaders nuevos
```

**Structure Decision**: Proyecto de escritorio único (no hay separación frontend/backend). Los
shaders viven en `assets/shaders/` (ya existente); se añade un subdirectorio `legacy/` para mover
ahí los 7 shaders parciales consolidados por el mega shader, preservándolos como referencia sin
borrarlos (Principio V: cambios acotados, sin pérdida de material de referencia). El código C++
afectado ya existe en `src/rendering/models/` y `src/rendering/opengl/`; esta feature extiende esos
archivos en vez de crear nuevos módulos.

## Complexity Tracking

> Fill ONLY if Constitution Check has violations that must be justified

Ninguna violación de la constitución identificada. No se requiere esta sección.
