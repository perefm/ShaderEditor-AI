# Feature Specification: Mega Shader de Materiales Assimp (PBR + Bones + 1 Luz)

**Feature Branch**: `008-mega-shader-pbr-lighting`
**Created**: 2026-09-09
**Status**: Implemented
**Input**: User description: "Mega shader / conjunto de shaders para dibujar objetos 3D con animación por bones, soporte de 1 luz, iluminación por píxel y todos los mapas de materiales que soporta Assimp, con fallback a color de material cuando no hay texturas; además evaluar shaders adicionales (toon shading) y ambient occlusion sin pasadas extra de render"

## Contexto técnico relevante (investigación previa)

- Ya existen shaders Phoenix-style independientes en `assets/shaders/`: `bone_animation.glsl`,
  `bone_animation_bump_mapping.glsl`, `bone_animation_material_only.glsl`, `bump_mapping.glsl`,
  `material_pixel_lighting.glsl`, `pbr_animation.glsl`, `pbr_animation_artistic.glsl`. Cada uno
  cubre una combinación parcial (con/sin bones, con/sin normal map, con/sin PBR), lo que obliga a
  elegir el shader "correcto" por modelo y provoca duplicación de código de iluminación.
- `pbr_animation.glsl` es el más completo actualmente: soporta skinning por bones (`gBones`,
  `aBoneID`/`aBoneWeight`), normal mapping tangente (`texture_normal1` + TBN), base
  color/metalness/roughness (con fallback a factores escalares vía `hasDiffuseTexture`/
  `hasPbrTextures`), emissive (textura + factor), transmission/opacity para materiales
  translúcidos tipo cristal, y una sola luz puntual (`lightPosition`/`lightColor`) con
  Cook-Torrance (GGX + Smith + Fresnel) y tone mapping Reinhard + gamma. No soporta ambient
  occlusion ni un modo "solo color de material sin ninguna textura" explícito (usa `Mat_Kd` como
  fallback de albedo, pero no hay fallback dedicado para specular/ambient sin PBR).
- `ModelMaterial` ([ModelDocument.h](/src/rendering/models/ModelDocument.h:55)) ya modela los
  flags de presencia por tipo de mapa (`hasDiffuseTexture`, `hasPbrTextures`, `hasNormalMap`,
  `hasEmissiveTexture`) y los factores/colores Phoenix (`Mat_Ka`/`Mat_Kd`/`Mat_Ks`/
  `Mat_KsStrenght`, `metallicFactor`, `roughnessFactor`, `emissiveFactor`, `transmissionFactor`,
  `opacity`). No existe todavía un flag para AO/occlusion texture ni para un lightmap.
- `AssimpModelLoader` es quien decide qué `aiTextureType` de Assimp se mapea a qué slot
  (`texture_diffuse1`, `texture_normal1`, `texture_metalness1`, `texture_roughness1`,
  `texture_emissive1`); Assimp expone más tipos que actualmente no se cargan (p.ej.
  `aiTextureType_AMBIENT_OCCLUSION`, `aiTextureType_LIGHTMAP`, `aiTextureType_SHEEN`,
  `aiTextureType_SPECULAR`, `aiTextureType_HEIGHT`/displacement clásico no-PBR).
- `PreviewRenderer` ([PreviewRenderer.cpp](/src/rendering/opengl/PreviewRenderer.cpp)) renderiza
  con **un único framebuffer offscreen** (`ensureFramebuffer`: una textura de color +
  renderbuffer depth/stencil combinado) y **una sola pasada de dibujo por frame** (no hay
  depth-prepass, ni G-Buffer, ni MRT/`glDrawBuffers` con múltiples destinos). Además, el adjunto
  de profundidad actual es un `GL_RENDERBUFFER` (`depthStencilBuffer_`), no una textura, por lo
  que ni siquiera podría muestrearse como `sampler2D` en un shader de post-proceso sin cambiarlo
  primero a `GL_TEXTURE_2D` vía `glFramebufferTexture2D(..., GL_DEPTH_ATTACHMENT, ...)`.
- La constitución del proyecto exige (Principio IV) preservar el ciclo de vida válido de OpenGL
  y no romper el render en su forma actual; y (Principio V) mantener los cambios acotados.

> **Fuera de alcance en esta spec**: Ambient occlusion (horneado o en tiempo real) queda
> explícitamente fuera del alcance de esta feature. Se evaluará en una spec separada, una vez
> analizado su impacto en la arquitectura de render de una sola pasada descrita arriba.

> **Fuera de alcance en esta spec**: Bokeh / Depth of Field (desenfoque de profundidad de campo
> controlado por sliders de distancia focal/rango/radio de blur) también queda explícitamente
> fuera de alcance. A diferencia del Toon Shading o el Rim Lighting (que solo necesitan datos ya
> disponibles por píxel en la pasada actual: normal, posición, color de material), un bokeh
> requiere leer, para cada píxel final, los píxeles vecinos ya renderizados de la escena
> ponderados por su profundidad — esto exige como mínimo dos pasadas de render (una pasada de
> escena a una textura de color + una textura de profundidad muestreable, seguida de una pasada de
> post-proceso de pantalla completa que aplique el blur variable), lo que rompe la arquitectura de
> una sola pasada / un único framebuffer actual de `PreviewRenderer`. Se evaluará en una spec
> separada de post-procesado multi-pasada (junto con SSAO en tiempo real, si procede).

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Renderizar cualquier modelo importado con un único shader coherente (Priority: P1)

Como usuario que importa modelos 3D vía Assimp (FBX, glTF, OBJ, DAE...), quiero que un mismo
"mega shader" sea capaz de dibujar correctamente el modelo tanto si tiene todos los mapas de
textura (base color, normal, metalness/roughness, emissive, specular, height) como si no tiene
ninguno y solo dispone de colores de material (Mat_Ka/Mat_Kd/Mat_Ks), sin tener que cambiar de
shader manualmente ni obtener artefactos visuales (texturas negras/garbage) por samplers sin
vincular.

**Why this priority**: Es el objetivo central de la petición: reducir la fragmentación actual de
7 shaders parciales a un flujo único y predecible, válido para el 100% de los modelos importados.

**Independent Test**: Cargar un modelo con todos los mapas (p.ej. el coche PBR usado en specs
archivadas 007) y otro modelo sin ninguna textura (solo colores de material) con el mismo shader
asignado; ambos deben renderizar con iluminación correcta y sin errores de sampler ni texturas en
negro.

**Acceptance Scenarios**:

1. **Given** un modelo con base color, normal map, metalness/roughness y emissive, **When** se
   renderiza con el mega shader, **Then** todos los mapas se muestran combinados correctamente
   (albedo, relieve, especular físico, autoiluminación).
2. **Given** un modelo cuyo material no tiene ninguna textura asignada, **When** se renderiza con
   el mismo mega shader, **Then** el resultado usa los colores de material (`Mat_Ka`/`Mat_Kd`/
   `Mat_Ks`/`Mat_KsStrenght`) como si fuera un material sólido, sin samplear texturas no
   vinculadas.
3. **Given** un modelo con solo algunos mapas (p.ej. solo base color, sin normal ni PBR), **When**
   se renderiza, **Then** los mapas ausentes caen a su fallback correspondiente (normal
   interpolado del vértice, factores escalares de metalness/roughness) sin degradar los mapas
   presentes.

---

### User Story 2 - Animación por bones combinada con cualquier combinación de materiales (Priority: P1)

Como usuario que importa modelos animados por esqueleto (skinning), quiero que el mismo mega
shader anime correctamente los vértices por bones a la vez que aplica cualquier combinación de
mapas de material, para no tener que mantener una copia "bone" y otra "no bone" de cada variante
de material.

**Why this priority**: La animación por bones es transversal a todas las combinaciones de mapas;
duplicar shaders por esta razón es la causa principal de la fragmentación actual (`bone_animation*`
vs el resto).

**Independent Test**: Cargar un modelo con esqueleto y textura completa, y otro modelo estático
(sin esqueleto, pesos de bone a 0) con el mismo shader; el estático debe comportarse igual que sin
skinning (transform identidad) y el animado debe deformarse correctamente frame a frame.

**Acceptance Scenarios**:

1. **Given** un modelo con esqueleto y pesos de bone válidos, **When** se reproduce una animación,
   **Then** los vértices se transforman con `gBones` por frame y el resto de mapas de material se
   aplican con normalidad sobre la malla deformada.
2. **Given** un modelo sin esqueleto (pesos de bone a 0 en todos los vértices), **When** se
   renderiza con el mismo shader, **Then** el resultado es idéntico a aplicar solo la matriz de
   modelo (sin deformación adicional).

---

### User Story 3 - Iluminación por píxel con una única luz consistente en todos los materiales (Priority: P1)

Como usuario, quiero que la iluminación (una luz, calculada por píxel) se comporte de forma
consistente independientemente de si el material usa el flujo PBR (metallic/roughness) o el flujo
clásico Phong/Blinn-Phong (colores de material sin texturas), para que la escena se vea coherente
al mezclar modelos de distinto origen.

**Why this priority**: Sin esta coherencia, mezclar un modelo PBR con un modelo "solo color" en la
misma escena produce saltos de brillo/contraste evidentes.

**Independent Test**: Colocar dos modelos lado a lado (uno con textura PBR completa, otro solo con
color de material) bajo la misma luz y verificar visualmente que ambos responden de forma
razonablemente equivalente a la dirección/intensidad de la luz (mismo `lightPosition`/
`lightColor`, mismo term de sombreado difuso, especular coherente en intensidad relativa).

**Acceptance Scenarios**:

1. **Given** una escena con una luz puntual, **When** se ilumina un material PBR sin textura de
   albedo (solo `baseColorFactor`), **Then** el resultado usa el mismo modelo de iluminación
   Cook-Torrance que un material PBR con textura, sustituyendo únicamente el término de albedo.
2. **Given** un material sin ningún dato PBR (solo colores clásicos Mat_Ka/Mat_Kd/Mat_Ks), **When**
   se ilumina, **Then** el shader aplica un modelo Blinn-Phong equivalente en lugar de forzar
   valores PBR por defecto poco representativos (p.ej. metallic=1.0 en un material que nunca fue
   pensado como metálico).

---

### User Story 4 - Soporte de mapas de material adicionales de Assimp (specular map, height/displacement clásico) (Priority: P2)

Como usuario, quiero que el mega shader (o el conjunto de shaders) reconozca también los mapas de
Assimp que hoy no se cargan (specular map dedicado, height map clásico para materiales no-PBR),
aplicándolos cuando el modelo los tenga y ignorándolos con seguridad cuando no existan.

> Nota: el ambient occlusion/lightmap queda explícitamente fuera de alcance de esta spec (ver
> sección "Fuera de alcance" arriba); se tratará en una spec futura dedicada.

**Why this priority**: Completa la cobertura "todos los mapas de materiales que soportan los
objetos de Assimp (si es que los tienen)" pedida explícitamente, más allá de los 5 mapas ya
soportados por `pbr_animation.glsl`.

**Independent Test**: Cargar un modelo FBX/OBJ con un specular map o un height map clásico
(`aiTextureType_SPECULAR`/`aiTextureType_HEIGHT`) y comprobar que se aplican; cargar un modelo sin
esos mapas y comprobar que no hay regresión.

**Acceptance Scenarios**:

1. **Given** un modelo con una textura de specular map dedicado (`aiTextureType_SPECULAR`),
   **When** se renderiza con el camino Blinn-Phong clásico, **Then** el término especular se
   modula por el valor muestreado de esa textura en vez del `Mat_Ks` constante.
2. **Given** un modelo sin specular map ni height map, **When** se renderiza, **Then** el shader
   usa los fallbacks actuales (`Mat_Ks`/normal de vértice) sin regresión visual.

---

### User Story 5 - Explorar shaders adicionales decisivos: Toon Shading (Priority: P3)

Como usuario, quiero disponer de un shader adicional de Toon Shading (cel shading) reutilizando la
misma infraestructura de materiales/bones/luz, para poder representar escenas con un estilo visual
no realista sin tener que rehacer la carga de materiales.

**Why this priority**: Mejora expresiva solicitada explícitamente, pero no bloquea el objetivo
principal (el mega shader realista); se apoya en la misma base de datos de materiales ya definida
en la User Story 1-3.

**Independent Test**: Aplicar el shader Toon a un modelo ya cargado (con o sin texturas) y verificar
que el resultado usa bandas de iluminación discretas (quantized diffuse) en vez de un degradado
continuo, más un contorno/silueta opcional.

**Acceptance Scenarios**:

1. **Given** un modelo con textura de base color, **When** se aplica el shader Toon, **Then** el
   color base se modula con bandas de luz discretas (p.ej. 3-4 niveles) en vez de un gradiente
   Phong continuo.
2. **Given** un modelo sin ninguna textura, **When** se aplica el shader Toon, **Then** se usa
   `Mat_Kd` como color base modulado por las mismas bandas discretas.

---

### User Story 6 - Explorar shaders adicionales decisivos: Rim Lighting / Fresnel Highlight (Priority: P3)

Como usuario, quiero disponer de un shader adicional que añada un realce de silueta (rim/Fresnel
lighting) sobre el mismo pipeline de materiales/bones/luz, para poder destacar la silueta de
personajes u objetos (uso típico en presentaciones, selección visual, efectos "hero object") sin
tener que recurrir a un post-proceso de varias pasadas.

**Why this priority**: Es una técnica de una sola pasada (se calcula con la normal y la dirección
de vista ya disponibles en el fragment shader), de bajo costo de implementación al reutilizar el
mega shader, y con alto valor para presentar modelos importados de forma más clara. Prioridad P3
porque, igual que el Toon shading, es una mejora expresiva adicional y no bloquea el objetivo
central.

**Independent Test**: Aplicar el shader Rim Lighting a un modelo ya cargado (con o sin texturas) y
comprobar visualmente que los bordes de la silueta (donde la normal es casi perpendicular a la
dirección de vista) se iluminan con un color/intensidad configurable, sin necesidad de ningún pase
de render adicional.

**Acceptance Scenarios**:

1. **Given** un modelo con textura de base color, **When** se aplica el shader Rim Lighting,
   **Then** el color base se combina con un término de realce en los bordes de silueta calculado
   por píxel (`1 - dot(normal, viewDir)` elevado a un exponente configurable).
2. **Given** un modelo sin ninguna textura, **When** se aplica el shader Rim Lighting, **Then** se
   usa `Mat_Kd` como color base y el realce de silueta se aplica igualmente.

---

## Organización y catálogo de shaders *(nueva sección)*

Para cumplir con la petición de organizar **todos** los shaders del proyecto de forma coherente
(no solo los nuevos), el plan DEBE establecer y documentar un catálogo con las siguientes
categorías. Cada shader existente o nuevo se cataloga en una única categoría; el nombre de archivo
final puede ajustarse en el plan para reflejar la categoría (p. ej. prefijos de carpeta o de
nombre), respetando el Principio V (cambios acotados) en cuanto a mover archivos innecesariamente.

1. **Shaders de aprendizaje / plantilla ("Getting Started")**: shaders mínimos usados como ejemplo
   inicial o plantilla en blanco del editor, sin dependencia de materiales Assimp.
   - `basic.glsl` (color plano vía uniform).
   - `textured.glsl` (una sola textura vía uniform `imageTexture`).
   - `pixel_lighting.glsl` (Blinn-Phong por píxel con normal derivada por `dFdx`/`dFdy`, sin usar
     el layout de vértice Phoenix; pensado para geometría procedural simple, no para modelos
     Assimp).
   - `diagnostics.glsl` (color blanco fijo, para verificar que la geometría/pipeline se ve).
   - `uniforms.glsl` (demuestra el uso de uniforms variados: `intensity`, `tint`).

2. **Shaders de materiales Assimp — realista (esta feature)**: shaders que consumen el layout de
   vértice Phoenix (`aPos`/`aNormal`/`aTexCoords`/`aTangent`/`aBiTangent`/`aBoneID`/`aBoneWeight`)
   y los uniforms de `ModelMaterial`, con foco en un resultado físicamente plausible.
   - El **mega shader** resultante de esta spec (User Stories 1-4), que sustituye/consolida a los
     shaders parciales actuales listados abajo como legacy.
   - *(Legacy, a deprecar/consolidar según el plan)*: `bone_animation.glsl`,
     `bone_animation_bump_mapping.glsl`, `bone_animation_material_only.glsl`, `bump_mapping.glsl`,
     `material_pixel_lighting.glsl`, `pbr_animation.glsl`, `pbr_animation_artistic.glsl`.

3. **Shaders de materiales Assimp — estilizados/artísticos (no fotorrealistas)**: shaders que
   comparten el mismo pipeline de materiales/bones/luz que el mega shader, pero sustituyen el
   modelo de shading final por un efecto de estilo.
   - Toon Shading / cel shading (User Story 5, nuevo).
   - Rim Lighting / Fresnel Highlight (User Story 6, nuevo).
   - `pbr_animation_artistic.glsl` (variante artística ya existente del PBR) se reevalúa en el
     plan: o se re-cataloga aquí explícitamente, o se consolida dentro del mega shader como un
     modo/uniform de estilo, evitando mantener dos archivos con lógica de iluminación duplicada.

El plan resultante DEBE incluir una tabla o listado equivalente que, para cada archivo final del
catálogo, indique: categoría, si soporta bones, si soporta PBR o solo colores clásicos, y qué mapas
de material admite — de forma que un usuario pueda elegir el shader correcto sin ambigüedad (esto
amplía, sin sustituir, el requisito ya existente FR-007 para el caso de que el mega shader deba
dividirse en varios archivos).

---

### Edge Cases

- ¿Qué ocurre cuando un material declara `hasPbrTextures=true` pero solo una de las dos texturas
  (metalness o roughness) está realmente vinculada (según cómo Assimp separe el canal combinado de
  glTF)? El shader no debe samplear un sampler no vinculado.
- ¿Qué ocurre con modelos que mezclan mallas con esqueleto y mallas estáticas dentro del mismo
  archivo (algunas con bones, otras sin ellos)? El shader único debe manejar ambos casos por malla
  sin fallos de skinning cruzado.
- ¿Qué pasa si un modelo no tiene ninguna luz configurada en la escena (valor por defecto de
  `lightPosition`/`lightColor`)? El material debe seguir siendo visible (no completamente negro)
  gracias al término ambiental/fallback existente.
- ¿Qué pasa si el número de huesos de un modelo excede `MAX_BONES` (100) del shader? Debe
  documentarse el límite y el comportamiento (recorte/fallo controlado), sin crash del proceso.
- ¿Qué pasa cuando se cambia de shader "realista" a "toon" o "rim lighting" para el mismo modelo en
  tiempo real? Los uniforms de material ya subidos (Phoenix naming) deben seguir siendo
  compatibles sin tener que re-cargar el modelo.
- ¿Podría esta feature romper el ciclo de vida OpenGL, la compilación de shaders o dejar la UI sin
  respuesta al añadir más samplers/uniforms por material? Debe verificarse que el número de
  texture units usados por material se mantiene dentro de límites razonables (típicamente <16 por
  fragment shader) en todas las combinaciones soportadas.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: El sistema DEBE proveer un único shader ("mega shader") de iluminación realista capaz
  de renderizar correctamente cualquier combinación de mapas de material soportados por Assimp
  (base color/diffuse, normal, metalness, roughness, emissive, specular map clásico,
  height/displacement clásico), incluyendo el caso de un material sin ninguna textura (solo color
  de material). El ambient occlusion queda fuera de alcance (ver sección "Fuera de alcance").
- **FR-002**: El mega shader DEBE soportar animación por vértices vía bones (skinning, hasta
  `kMaxBonesPerVertex`=4 influencias por vértice y `MAX_BONES`=100 huesos), reutilizando el mismo
  layout de vértice Phoenix (`aPos`/`aNormal`/`aTexCoords`/`aTangent`/`aBiTangent`/`aBoneID`/
  `aBoneWeight`) ya usado por los shaders existentes, y DEBE producir el mismo resultado que la
  matriz de modelo simple cuando el peso total de bones de un vértice es 0 (mallas sin esqueleto).
- **FR-003**: El mega shader DEBE soportar exactamente una fuente de luz (`lightPosition`/
  `lightColor`), calculada por píxel (fragment shader), no interpolada desde el vertex shader.
- **FR-004**: Para cada mapa de material soportado, el shader DEBE tener un flag booleano de
  presencia (siguiendo el patrón `hasXxx` ya usado, p.ej. `hasDiffuseTexture`, `hasNormalMap`,
  `hasEmissiveTexture`, `hasPbrTextures`) y DEBE usar un valor/factor de fallback en lugar de
  samplear el sampler correspondiente cuando el flag es falso.
- **FR-005**: El shader DEBE seleccionar automáticamente, sin intervención del usuario ni recarga
  de modelo, entre un camino de shading PBR (metallic-roughness, cuando el material aporta
  factores o texturas PBR) y un camino Blinn-Phong clásico (cuando el material solo aporta
  `Mat_Ka`/`Mat_Kd`/`Mat_Ks`/`Mat_KsStrenght`), de forma que ambos caminos respondan a la misma
  luz de forma visualmente coherente (mismo term difuso de base, especular proporcional a la
  intensidad configurada).
- **FR-006**: El sistema DEBE extender `ModelMaterial` y `AssimpModelLoader` para reconocer y
  cargar, cuando existan en el modelo de origen, los mapas de Assimp actualmente no soportados:
  specular map dedicado (`aiTextureType_SPECULAR`) y height/displacement clásico
  (`aiTextureType_HEIGHT`/`aiTextureType_DISPLACEMENT`), cada uno con su flag `hasXxx`
  correspondiente.
- **FR-007**: Si no es viable cubrir todas las combinaciones (skinning × PBR-vs-clásico ×
  presencia/ausencia de cada mapa) en un único archivo de shader por limitaciones prácticas
  (p.ej. límite de samplers, legibilidad, tiempo de compilación), el sistema DEBE dividirse en un
  conjunto reducido y documentado de shaders, cada uno con un comentario/README que explique
  exactamente para qué combinación de casos (con/sin bones, PBR/clásico) está pensado cada
  archivo, de forma que el usuario pueda elegir el correcto sin ambigüedad.
- **FR-008**: El sistema DEBE proponer y documentar en el plan al menos un shader adicional de Toon
  Shading (cel shading) que reutilice el mismo pipeline de materiales, bones y luz única descrito
  arriba (bandas de iluminación discretas en vez de degradado continuo), como shader
  seleccionable adicional (no sustituye al mega shader realista).
- **FR-009**: El sistema DEBE proponer y documentar en el plan un shader adicional de Rim
  Lighting/Fresnel Highlight que reutilice el mismo pipeline de materiales, bones y luz única
  (realce de silueta calculado por píxel a partir de la normal y la dirección de vista, sin
  pasadas de render adicionales), como shader seleccionable adicional.
- **FR-010**: El sistema DEBE reorganizar el catálogo completo de shaders del proyecto (incluidos
  los shaders básicos no ligados a Assimp: `basic.glsl`, `textured.glsl`, `pixel_lighting.glsl`,
  `diagnostics.glsl`, `uniforms.glsl`) en categorías coherentes y documentadas (p. ej. "shaders de
  aprendizaje/plantilla", "shaders de materiales Assimp", "shaders de estilo/artísticos"), de forma
  que el propósito y las diferencias entre todos los shaders del repositorio —no solo los nuevos—
  queden claros para el usuario. Ver sección "Organización y catálogo de shaders" más arriba.
- **FR-011**: El sistema DEBE preservar el comportamiento actual de materiales translúcidos
  (`transmissionFactor`/`opacity`, ver `pbr_animation.glsl`) dentro del mega shader, sin
  regresiones sobre modelos glTF con cristal/transparencia ya soportados.
- **FR-012**: El sistema MUST work on the intended supported platforms (Windows/otras plataformas
  de escritorio definidas por el proyecto) sin comportamiento específico de plataforma en el
  shading en sí.
- **FR-013**: El sistema MUST preservar los patrones de UI existentes para selección/asignación de
  shader por modelo, salvo que el plan defina explícitamente un cambio aprobado (p.ej. nuevas
  entradas en el selector de shaders para el mega shader, el shader Toon y el shader Rim Lighting).
- **FR-014**: El sistema MUST evitar romper el ciclo de vida de renderizado OpenGL (compilación de
  shaders, carga de recursos, bucle de frame) o dejar la UI principal sin respuesta durante el uso
  normal, incluyendo al compilar/cargar el mega shader con el máximo de mapas soportados
  simultáneamente.
- **FR-015**: El ambient occlusion (horneado o en tiempo real) queda explícitamente FUERA DE
  ALCANCE de esta feature; no debe implementarse como parte de este trabajo. Se abordará, si
  procede, en una spec futura separada que analice primero su impacto en la arquitectura de
  render de una sola pasada.
- **FR-016**: El efecto Bokeh/Depth of Field (sliders de distancia focal, rango de nitidez y radio
  máximo de blur) queda explícitamente FUERA DE ALCANCE de esta feature; no debe implementarse
  como parte de este trabajo, al requerir como mínimo dos pasadas de render (ver "Contexto técnico
  relevante"). Se abordará, si procede, en una spec futura separada de post-procesado
  multi-pasada que documente primero los cambios de arquitectura necesarios (framebuffer
  adicional, adjunto de profundidad como textura muestreable, pase de post-proceso de pantalla
  completa).

### Key Entities *(include if feature involves data)*

- **ModelMaterial (extendido)**: representa las propiedades de material de una malla importada.
  Además de los campos ya existentes (colores Phoenix, factores PBR, flags de presencia de
  diffuse/PBR/normal/emissive, transmission/opacity), añade flags y datos para los mapas nuevos:
  specular map dedicado y height/displacement clásico.
- **ModelTextureSlot**: slot de textura ya existente (nombre de uniform Phoenix, ruta origen o
  datos embebidos, id de textura GL); se reutiliza sin cambios estructurales para los mapas
  nuevos.
- **Mega Shader (o conjunto de shaders documentado)**: artefacto GLSL (o conjunto) que consume
  `ModelMaterial` y el layout de vértice Phoenix, produce el color final por píxel combinando
  skinning opcional, una luz, y todos los mapas soportados (o sus fallbacks).
- **Shader Toon**: variante adicional que reutiliza el mismo modelo de materiales/luz/bones pero
  sustituye el shading final por bandas discretas de iluminación.
- **Shader Rim Lighting**: variante adicional que reutiliza el mismo modelo de materiales/luz/bones
  pero añade un término de realce de silueta basado en la normal y la dirección de vista.
- **Catálogo de shaders**: documento/tabla (ver "Organización y catálogo de shaders") que agrupa
  todos los shaders del repositorio (nuevos y existentes) en categorías coherentes, indicando
  soporte de bones, PBR/clásico y mapas de material por archivo.

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Un mismo modelo con todos los mapas soportados y otro sin ninguna textura se
  renderizan correctamente (sin texturas negras/garbage, sin errores de compilación de shader)
  usando la misma familia de shader(s) documentada, verificable por inspección visual y por logs
  libres de errores de sampler.
- **SC-002**: El número de shaders "parciales" necesarios para cubrir todas las combinaciones
  soportadas se reduce respecto a los 7 archivos parciales actuales (`bone_animation*`,
  `bump_mapping`, `material_pixel_lighting`, `pbr_animation*`), documentando explícitamente cuántos
  shaders finales existen y para qué sirve cada uno.
- **SC-003**: Un modelo con esqueleto animado y un modelo estático (sin esqueleto) usan el mismo
  shader del conjunto sin necesitar variantes distintas por esa sola razón.
- **SC-004**: Los mapas de Assimp actualmente no soportados (specular map dedicado,
  height/displacement clásico) se cargan y aplican correctamente en al menos un modelo de prueba
  real que los contenga.
- **SC-005**: El catálogo de shaders (ver "Organización y catálogo de shaders") documenta todos los
  archivos de `assets/shaders/` existentes tras esta feature, agrupados en categorías coherentes,
  sin archivos huérfanos sin categorizar.
- **SC-006**: El shader Toon Shading funciona sobre al menos un modelo con textura y uno sin
  textura, mostrando bandas de iluminación discretas verificables visualmente.
- **SC-006b**: El shader Rim Lighting funciona sobre al menos un modelo con textura y uno sin
  textura, mostrando el realce de silueta verificable visualmente, sin pasadas de render
  adicionales.
- **SC-007**: El flujo de carga/renderizado del modelo con el mega shader mantiene un bucle de
  render válido y una UI responsiva en las plataformas soportadas, sin crashes ni cuelgues durante
  el uso normal (carga de modelo, cambio de shader, reproducción de animación).

## Assumptions

- Se asume que "todos los mapas de materiales que soportan los objetos de Assimp (si es que los
  tienen)" se refiere a los `aiTextureType` relevantes para el modelo PBR metallic-roughness y
  Phong clásico ya usados por Phoenix/este proyecto (diffuse/base color, normal, metalness,
  roughness, emissive, specular, height/displacement), y no a tipos exóticos poco usados en la
  práctica (sheen, clearcoat, anisotropy, sheen roughness) ni a ambient occlusion/lightmap (fuera
  de alcance, ver FR-015), que quedan fuera de alcance salvo que aparezcan en un modelo real usado
  como caso de prueba.
- Se asume que el efecto Bokeh/Depth of Field, al requerir una arquitectura de post-proceso
  multi-pasada (ver FR-016), queda completamente fuera del alcance de esta feature; no se
  implementará ni siquiera de forma parcial/aproximada en una sola pasada.
- Se asume que "1 luz" significa una única fuente de luz activa por escena (puntual, con
  `lightPosition`/`lightColor` como en los shaders actuales), no un sistema multi-luz; añadir
  soporte multi-luz queda fuera de alcance de esta feature.
- Se asume que el conjunto de shaders actuales (`bone_animation*.glsl`, `bump_mapping.glsl`,
  `material_pixel_lighting.glsl`, `pbr_animation*.glsl`) puede quedar deprecated/reemplazado por
  el(los) shader(s) nuevo(s), pero no se asume su borrado automático: la decisión de mantenerlos
  como referencia o eliminarlos se deja para el plan/implementación, respetando el Principio V
  (cambios acotados) de la constitución. Los shaders de aprendizaje/plantilla (`basic.glsl`,
  `textured.glsl`, `pixel_lighting.glsl`, `diagnostics.glsl`, `uniforms.glsl`) se mantienen sin
  cambios funcionales, solo re-catalogados documentalmente.
- Se asume que el shader Toon Shading y el shader Rim Lighting son propuestas adicionales
  priorizadas por debajo del mega shader realista (P3), y que cada uno puede implementarse como un
  shader independiente que reutiliza los mismos uniforms/atributos, no como parte del mismo
  archivo que el mega shader realista.
- Se asume que las plataformas soportadas y los requisitos de OpenGL (contexto, versión GLSL 460
  ya usada) son los mismos que en el resto del proyecto; esta feature no introduce nuevos
  requisitos de plataforma.
