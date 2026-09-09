# Data Model: Mega Shader de Materiales Assimp

## ModelMaterial (extensión de `src/rendering/models/ModelDocument.h`)

Campos ya existentes (sin cambios):

| Campo | Tipo | Uniform GLSL | Notas |
|---|---|---|---|
| `colorAmbient` | `glm::vec3` | `Mat_Ka` | Ambiente clásico |
| `colorDiffuse` | `glm::vec3` | `Mat_Kd` | Diffuse clásico / fallback de albedo PBR |
| `colorSpecular` | `glm::vec3` | `Mat_Ks` | Specular clásico |
| `specularStrength` | `float` | `Mat_KsStrenght` | Exponente/fuerza specular clásico |
| `metallicFactor` | `float` | `metallicFactor` | Factor PBR (default 1.0) |
| `roughnessFactor` | `float` | `roughnessFactor` | Factor PBR (default 1.0) |
| `hasPbrTextures` | `bool` | `hasPbrTextures` | Hay textura metalness y/o roughness |
| `hasDiffuseTexture` | `bool` | `hasDiffuseTexture` | Hay textura base color/diffuse |
| `hasNormalMap` | `bool` | `hasNormalMap` | Hay textura normal |
| `hasEmissiveTexture` | `bool` | `hasEmissiveTexture` | Hay textura emissive |
| `emissiveFactor` | `glm::vec3` | `emissiveFactor` | Factor emissive |
| `transmissionFactor` | `float` | `transmissionFactor` | KHR_materials_transmission |
| `opacity` | `float` | `materialOpacity` | Alpha base |
| `textureSlots` | `std::vector<ModelTextureSlot>` | (por nombre) | Todas las texturas del material |

Campos **nuevos** a añadir:

| Campo | Tipo | Uniform GLSL | Notas |
|---|---|---|---|
| `hasPbrWorkflow` | `bool` | `hasPbrWorkflow` | `true` si el material define explícitamente `AI_MATKEY_METALLIC_FACTOR`/`ROUGHNESS_FACTOR` o tiene textura PBR; decide el camino de shading (Decisión 2 de research.md) |
| `hasSpecularMap` | `bool` | `hasSpecularMap` | Hay textura `texture_specular1` (`aiTextureType_SPECULAR`) |
| `hasHeightMap` | `bool` | `hasHeightMap` | Hay textura `texture_height1` (`aiTextureType_HEIGHT`/`DISPLACEMENT`) |

Los slots de textura nuevos (`texture_specular1`, `texture_height1`) usan la misma
`ModelTextureSlot` existente; no se necesita una nueva estructura.

## Validación de equivalencia de materiales (`AssimpModelLoader`)

La función interna que compara dos `ModelMaterial` para deduplicar (usada en la carga, ver
`AssimpModelLoader.cpp` líneas ~378-381) DEBE extenderse para comparar también `hasPbrWorkflow`,
`hasSpecularMap` y `hasHeightMap`, de forma que dos materiales que solo difieran en estos campos
nuevos no se fusionen incorrectamente en la deduplicación.

## Uniforms nuevos del mega shader (`assets/shaders/mega_material.glsl`)

Además de los uniforms ya usados por `pbr_animation.glsl` (MVP, model, gBones, lightPosition,
lightColor, uCameraPos, Mat_Ka/Kd/Ks/KsStrenght, metallicFactor, roughnessFactor,
hasPbrTextures, hasDiffuseTexture, hasNormalMap, hasEmissiveTexture, emissiveFactor,
transmissionFactor, materialOpacity, texture_diffuse1, texture_normal1, texture_metalness1,
texture_roughness1, texture_emissive1):

| Uniform | Tipo | Default | Uso |
|---|---|---|---|
| `hasPbrWorkflow` | `bool` | `false` | Selecciona Cook-Torrance vs. Blinn-Phong. **Read-only**: subido cada frame por `PreviewRenderer::bindMeshMaterial`; registrado en `UniformIntrospectionService::isPhoenixAutoUniform` para que el panel de Uniforms no lo muestre como editable. |
| `hasSpecularMap` | `bool` | `false` | Modula `Mat_Ks`/specular clásico por textura. **Read-only** (idem). |
| `texture_specular1` | `sampler2D` | (sin bind) | Specular map clásico, solo muestreado si `hasSpecularMap` |
| `hasHeightMap` | `bool` | `false` | Aplica desplazamiento/parallax simple (o solo se documenta como reservado si no se implementa parallax completo, ver quickstart.md). **Read-only** (idem). |
| `texture_height1` | `sampler2D` | (sin bind) | Height/displacement map clásico |
| `ambientStrength` | `float` | `1.0` | Reutilizado del camino clásico (`material_pixel_lighting.glsl`) para el término ambiental `Mat_Ka * ambientStrength`. Editable por el usuario (no es un dato de material auto-subido). |

## Shaders de estilo (Toon, Rim Lighting)

Ambos reutilizan exactamente el mismo bloque de atributos de vértice, uniforms de material y
uniforms de luz que `mega_material.glsl` (mismo vertex shader, solo cambia el fragment shader),
para que el motor pueda subir los mismos uniforms sin lógica condicional por shader.

| Shader | Uniforms adicionales propios | Notas |
|---|---|---|
| `toon_material.glsl` | `toonBands` (`int`, default 4) | Número de bandas discretas de iluminación |
| `rim_lighting_material.glsl` | `rimColor` (`vec3`, default blanco), `rimPower` (`float`, default 2.0) | Color e intensidad del realce de silueta |

## Catálogo de shaders (entidad documental)

Ver [contracts/shader-catalog.md](/specs/008-mega-shader-pbr-lighting/contracts/shader-catalog.md)
para la tabla completa por archivo (categoría, soporte de bones, PBR/clásico, mapas admitidos).
