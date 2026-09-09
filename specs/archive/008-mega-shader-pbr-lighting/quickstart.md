# Quickstart: Mega Shader de Materiales Assimp

Guía rápida para probar manualmente esta feature una vez implementada.

## 1. Compilar

```powershell
cmake --build build-vcpkg --config Debug
```

## 2. Probar el mega shader con un modelo animado y con texturas

1. Abrir `ShaderEditor`.
2. Cargar un modelo con esqueleto y texturas (por ejemplo `assets/models/Fox/Fox.glb`, usado ya
   en los tests).
3. Abrir shader: `assets/shaders/mega_material.glsl`.
4. Verificar: el modelo se ve texturizado (diffuse/normal si el modelo los trae), y si tiene
   animación, reproducirla y comprobar que el skinning se aplica correctamente.

## 3. Probar el fallback a color de material (sin texturas)

1. Cargar un modelo o primitiva sin ninguna textura asignada (o un modelo OBJ solo con colores
   Ka/Kd/Ks).
2. Con `mega_material.glsl` activo, verificar que el objeto se dibuja con los colores de material
   (`Mat_Ka`/`Mat_Kd`/`Mat_Ks`) y iluminación Blinn-Phong clásica (no negro, no PBR con roughness
   por defecto rara).

## 4. Probar selección automática PBR vs. clásico

1. Cargar el modelo de coche PBR de la spec 007 (con `metallicFactor`/`roughnessFactor` glTF).
2. Verificar que se ve con Cook-Torrance (highlights especulares físicamente plausibles,
   metal/dieléctrico correctos), y que transmission/opacity del vidrio se preservan (no regresión
   de la spec 007).
3. Cargar un modelo FBX/OBJ clásico (sin datos glTF) y verificar que usa Blinn-Phong (aspecto
   "clásico", no PBR).

## 5. Probar Toon Shading y Rim Lighting

1. Abrir `assets/shaders/toon_material.glsl` sobre cualquier modelo con textura; verificar bandas
   discretas de iluminación (no degradado continuo).
2. Abrir `assets/shaders/rim_lighting_material.glsl`; verificar un halo/borde luminoso en el
   contorno del objeto visto desde cámara, más intenso en los bordes que de frente.

## 6. Ejecutar tests automáticos

```powershell
ctest --test-dir build-vcpkg -C Debug --output-on-failure
```

Deben pasar (nuevos/actualizados):
- `test_assimp_model_loader.cpp`: casos de `hasSpecularMap`/`hasHeightMap`/`hasPbrWorkflow`.
- `test_example_shaders.cpp`: carga de `mega_material.glsl`, `toon_material.glsl`,
  `rim_lighting_material.glsl` vía `ShaderFileService` sin error.

## 7. Confirmar que no hay regresión de shaders legacy

Los archivos movidos a `assets/shaders/legacy/` deben seguir siendo abribles manualmente (File >
Open shader...) y compilar igual que antes de moverlos (solo cambia la ruta, no el contenido).
