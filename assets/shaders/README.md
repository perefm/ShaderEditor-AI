# Catálogo de shaders (`assets/shaders/`)

Este proyecto organiza sus shaders en tres categorías. Consulta también
[specs/archive/008-mega-shader-pbr-lighting/contracts/shader-catalog.md](/specs/archive/008-mega-shader-pbr-lighting/contracts/shader-catalog.md)
para el contrato de diseño original.

## Categoría 1 — Aprendizaje / plantilla

Shaders simples usados como ejemplo pedagógico o plantilla de partida; no dependen de datos de
material Assimp ni de bones.

| Archivo | Propósito |
|---|---|
| `basic.glsl` | Shader mínimo de color plano; plantilla base del editor. |
| `textured.glsl` | Ejemplo de muestreo de una textura simple. |
| `pixel_lighting.glsl` | Ejemplo de iluminación por píxel con una luz, sin material Assimp. |
| `diagnostics.glsl` | Visualización de normales/UVs/otros buffers para depuración. |
| `uniforms.glsl` | Ejemplo de uso de uniforms variados (tiempo, resolución, etc.). |

## Categoría 2 — Materiales Assimp realistas

| Archivo | Propósito |
|---|---|
| `mega_material.glsl` | **Shader recomendado por defecto** para dibujar cualquier modelo importado por Assimp. Soporta bones (con fallback estático), PBR metallic-roughness y Blinn-Phong clásico (selección automática por material), y todos los mapas: base color/diffuse, normal, metalness, roughness, emissive, specular, height. Usa colores de material (`Mat_Ka/Kd/Ks`) cuando no hay textura para un canal. Una única pasada de render, una luz. |
| `legacy/bone_animation.glsl` | (Consolidado en `mega_material.glsl`.) Skinning + iluminación básica sin mapas PBR. Conservado como referencia. |
| `legacy/bone_animation_bump_mapping.glsl` | (Consolidado.) Skinning + normal mapping sin PBR. |
| `legacy/bone_animation_material_only.glsl` | (Consolidado.) Skinning + solo colores de material, sin texturas. |
| `legacy/bump_mapping.glsl` | (Consolidado.) Normal mapping sin bones. |
| `legacy/material_pixel_lighting.glsl` | (Consolidado.) Blinn-Phong clásico por píxel con colores de material. |
| `legacy/pbr_animation.glsl` | (Consolidado.) Precursor directo de `mega_material.glsl`; PBR completo + bones, pero sin specular/height map dedicados. |

## Categoría 3 — Materiales Assimp estilizados / artísticos

| Archivo | Propósito |
|---|---|
| `toon_material.glsl` | Cel shading: bandas discretas de iluminación (`toonBands`) sobre el mismo pipeline de datos (bones + mapas) que `mega_material.glsl`. |
| `rim_lighting_material.glsl` | Realce de silueta tipo Fresnel (`rimColor`/`rimPower`) sobre el mismo pipeline de datos. |
| `legacy/pbr_animation_artistic.glsl` | Variante artística previa de PBR; conservada como referencia, sin fusionar con los shaders estilizados nuevos. |

## Notas de alcance futuro (no implementado)

- **Ambient occlusion horneado** (textura AO precalculada, `aiTextureType_AMBIENT_OCCLUSION`):
  viable sin cambios de arquitectura de render (el `PreviewRenderer` ya dibuja en una única
  pasada; una textura de AO horneada es solo una textura más a samplear). Candidato para una spec
  futura que extienda `mega_material.glsl` con un uniform `hasAmbientOcclusionMap` /
  `texture_ambientoclussion1`.
- **SSAO en tiempo real** y **Bokeh/Depth of Field**: requieren un depth attachment sampleable
  (textura, no renderbuffer) y un segundo pase de post-proceso; fuera de alcance mientras
  `PreviewRenderer` mantenga su arquitectura de un único framebuffer/una única pasada.
