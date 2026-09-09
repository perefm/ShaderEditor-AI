# Research: Mega Shader de Materiales Assimp

## Decisión 1: Un único archivo GLSL vs. varios shaders

**Decisión**: Un único archivo `mega_material.glsl` (vertex + fragment) que cubre todas las
combinaciones (con/sin bones, PBR/clásico, con/sin cada mapa) mediante ramas condicionadas por
uniforms `hasXxx` (patrón ya usado en `pbr_animation.glsl`).

**Justificación**:
- El layout de vértice Phoenix es idéntico en todos los shaders de materiales Assimp
  (`aPos`/`aNormal`/`aTexCoords`/`aTangent`/`aBiTangent`/`aBoneID`/`aBoneWeight`), así que el
  vertex shader no necesita variantes: el skinning con fallback a pose sin deformar
  (`totalWeight > 0.0 ? boneTransform : mat4(1.0)`) ya es la misma lógica que
  `bone_animation_material_only.glsl` usa para "modelos estáticos que reutilizan el shader
  animado".
- El número de samplers necesarios en el peor caso (diffuse, normal, metalness, roughness,
  emissive, specular, height = 7 samplers) está muy por debajo del mínimo garantizado de unidades
  de textura de fragment shader en OpenGL 4.6 (`GL_MAX_TEXTURE_IMAGE_UNITS` ≥ 16), así que no hay
  razón técnica de límite de samplers para dividir en varios archivos.
- Todas las ramas (`if (hasNormalMap) {...}`, `if (hasPbrTextures) {...}`) son ramas uniformes
  (constantes por draw call, no por hilo/fragmento divergente dentro del mismo draw), por lo que
  el coste de las ramas no usadas es despreciable en hardware moderno (GPU descarta la rama vía
  predicción/branch coherente al ser un uniform, no `varying`).
- Precedente: `pbr_animation.glsl` ya demuestra que ~90% de esta lógica combinada compila y
  funciona en un único archivo hoy.

**Alternativas consideradas**:
- Mantener separados "shader PBR" y "shader clásico" (dos archivos): rechazado porque la User
  Story 3 exige selección automática sin intervención del usuario ni recarga, lo que requiere que
  ambos caminos convivan en el mismo programa para decidir en tiempo de shading, no en tiempo de
  selección de archivo.
- Generar variantes por combinatoria de `#define` (shader permutations): rechazado por
  complejidad de build adicional no justificada para ~7 mapas opcionales; el proyecto no tiene hoy
  infraestructura de preprocesado de shaders con macros condicionales, y añadirla violaría el
  Principio V (cambios acotados).

## Decisión 2: Selección automática PBR vs. Blinn-Phong clásico

**Decisión**: El fragment shader decide por material (no por usuario) qué camino de shading usar,
basado en si el material aporta señal PBR: `hasPbrTextures || metallicFactor` (siempre presente,
con default 1.0 si Assimp no publica el metallic factor) no es suficiente por sí solo para
diferenciar "material Phong clásico" de "material PBR sin texturas", porque
`AI_MATKEY_METALLIC_FACTOR`/`AI_MATKEY_ROUGHNESS_FACTOR` no siempre están presentes en materiales
no-glTF (OBJ/FBX clásicos). Por tanto, la señal decisiva es un nuevo flag agregado
`hasPbrWorkflow` calculado en `AssimpModelLoader` en tiempo de carga: `true` si el material define
`AI_MATKEY_METALLIC_FACTOR` o `AI_MATKEY_ROUGHNESS_FACTOR` explícitamente, o si tiene alguna
textura PBR (`hasPbrTextures`); `false` en otro caso (material puramente clásico
Ka/Kd/Ks/KsStrenght, típico de OBJ/FBX/DAE sin datos glTF).

**Justificación**: Assimp devuelve `AI_SUCCESS` de `material->Get(AI_MATKEY_METALLIC_FACTOR, ...)`
solo si la clave está presente en el material de origen; se puede capturar ese resultado (en vez
de ignorarlo como hace hoy el código) para poblar `hasPbrWorkflow` sin heurísticas frágiles.

**Alternativas consideradas**:
- Heurística basada en si `metallicFactor` != valor por defecto: rechazada, frágil (un material
  PBR legítimo podría tener metallicFactor=1.0 exactamente, igual que el default).
  Preferible usar el resultado explícito de `aiReturn` de Assimp.
- Dejar que el usuario elija manualmente PBR/clásico por uniform: rechazado, la User Story 3 pide
  explícitamente que sea automático.

## Decisión 3: Coherencia de iluminación entre caminos PBR y clásico

**Decisión**: Ambos caminos comparten el mismo cálculo de `lightDir`, `viewDir`, `normal` (con
normal mapping aplicado igual en ambos) y el mismo término difuso base `max(dot(normal,
lightDir), 0.0)`. El camino PBR usa Cook-Torrance (GGX+Smith+Fresnel) tal como
`pbr_animation.glsl`; el camino clásico usa Blinn-Phong tal como `material_pixel_lighting.glsl`
(specular con exponente `Mat_KsStrenght`, ambiente `Mat_Ka * ambientStrength`). No se intenta
unificar ambos modelos matemáticamente (serían visualmente distintos por diseño: PBR físicamente
plausible, Phong estilizado clásico); la coherencia exigida por la spec (FR-005/US3) se limita a
que ambos respondan a la misma posición/color de luz con intensidad relativa comparable, no a que
produzcan resultados numéricamente idénticos.

**Justificación**: Intentar forzar valores PBR "traducidos" desde Ka/Kd/Ks produciría resultados
visualmente peores que el Blinn-Phong original (que es como estos materiales fueron diseñados
para verse en primer lugar, en flujos que no son PBR).

## Decisión 4: Ambient occlusion y Bokeh — confirmación de exclusión

**Investigación de arquitectura de render** (requisito FR-015/FR-016 de la spec, resuelto en esta
fase de research en vez de en implementación):

- `PreviewRenderer::ensureFramebuffer` crea un único framebuffer con una textura de color
  (`GL_RGBA8`) y un renderbuffer combinado depth/stencil (`GL_DEPTH24_STENCIL8`, adjuntado vía
  `glFramebufferRenderbuffer`, no vía `glFramebufferTexture2D`). Un renderbuffer **no puede
  vincularse como `sampler2D`** en un shader; solo una textura puede.
- `renderFrame`/`renderModelFrame` ejecutan un único `glDrawElements`/pase de dibujo por mesh, sin
  pase de post-proceso de pantalla completa (no hay un segundo draw call de un quad full-screen
  en ningún punto del código).
- **Ambient occlusion horneado** (textura de AO precalculada por el DCC/modelo, ya soportada por
  Assimp vía `aiTextureType_AMBIENT_OCCLUSION`/`aiTextureType_LIGHTMAP`) SÍ sería técnicamente
  viable sin pasadas adicionales (es solo una textura más a samplear en la pasada única
  existente, exactamente igual que normal/metalness/roughness). Sin embargo, queda **fuera de
  alcance de esta spec** por decisión explícita del usuario (no de limitación técnica), y se
  deja documentado aquí para una spec futura que sí quiera cubrirlo.
- **SSAO en tiempo real** y **Bokeh/Depth of Field** SÍ requieren cambios de arquitectura
  (mínimo: depth attachment como textura + un segundo pase de post-proceso), confirmando que están
  correctamente excluidos de esta spec.

**Conclusión para el plan**: no se implementa ninguna forma de AO ni de Bokeh en esta feature. La
excepción documentada (AO horneado sería viable sin cambios de arquitectura) se dejará anotada en
`assets/shaders/README.md` como nota para una spec futura, sin implementarla aquí.

## Decisión 5: Reorganización del catálogo de shaders

**Decisión**: Mover los 7 shaders legacy a `assets/shaders/legacy/` (en vez de borrarlos),
mantener los 5 shaders de aprendizaje/plantilla donde están (`assets/shaders/*.glsl`, raíz), y
añadir los 3 shaders nuevos (`mega_material.glsl`, `toon_material.glsl`,
`rim_lighting_material.glsl`) también en la raíz de `assets/shaders/`, con un `README.md` nuevo
que documente el catálogo completo (ver `contracts/shader-catalog.md` de esta spec para el
contenido exacto).

**Justificación**: Mover a un subdirectorio en vez de borrar preserva referencia histórica
(Principio V) sin ambigüedad de cuáles son los shaders "vigentes" (los de la raíz) vs. legacy.
`ShaderFileService`/`Application` cargan shaders por ruta explícita elegida por el usuario (File >
Open shader...), así que mover archivos de carpeta no rompe ningún flujo de carga automática (no
hay una lista hardcodeada de rutas salvo `exampleShaderPath_`/`pixelLightingShaderPath_`, que no
apuntan a ninguno de los 7 legacy).

**Riesgo verificado**: se comprobó (`grep` sobre `src/`) que ningún archivo legacy está
referenciado por ruta hardcodeada en el código C++ (solo se mencionan en comentarios explicativos
dentro de `pbr_animation.glsl`, `ModelDocument.h`, `PreviewRenderer.cpp`); mover los archivos de
carpeta es seguro. Los comentarios que citan `pbr_animation.glsl` por nombre se actualizarán para
referenciar `mega_material.glsl` donde corresponda.
