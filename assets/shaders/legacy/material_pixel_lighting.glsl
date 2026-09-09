#type vertex
#version 460 core

// Phoenix-style vertex layout for imported (Assimp) meshes. The bone attributes are declared so
// the layout still matches meshes that carry them, but they are deliberately unused: this shader
// has no skinning path, and static/keyframe-animated models get their movement from the engine's
// per-mesh "model" matrix instead.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBiTangent;

uniform mat4 MVP;
uniform mat4 model;

out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    // Lighting is evaluated per pixel in world space, so the interpolated normal is left
    // unnormalized here and renormalized in the fragment stage.
    vNormal = mat3(model) * aNormal;
    vWorldPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = MVP * vec4(aPos, 1.0);
}


#type fragment
#version 460 core

out vec4 FragColor;

in vec3 vNormal;
in vec3 vWorldPos;

// Per-mesh material colors, uploaded by the engine using Phoenix's exact uniform names
// (see Mesh::setMaterialShaderVars). No textures are sampled: shading comes entirely from
// these material properties.
uniform vec3 Mat_Ka;                        // ambient color
uniform vec3 Mat_Kd = vec3(0.8, 0.8, 0.8);  // diffuse color
uniform vec3 Mat_Ks;                        // specular color
uniform float Mat_KsStrenght;               // specular exponent (shininess)

// Engine-provided position of the active camera (free camera or selected scene camera).
uniform vec3 uCameraPos;

uniform vec3 lightPosition = vec3(3.0, 4.0, 5.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform float ambientStrength = 1.0;

void main() {
    // Per-pixel (Blinn-Phong) lighting: every term below is evaluated per fragment rather than
    // interpolated from the vertices, so highlights stay tight even on low-poly meshes.
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(lightPosition - vWorldPos);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diffuseTerm = max(dot(normal, lightDir), 0.0);

    // Only add a highlight where the surface actually faces the light, and guard the exponent so
    // a material with no authored shininess does not produce pow(x, 0.0) == 1.0 everywhere.
    float specularTerm = (Mat_KsStrenght > 0.0 && diffuseTerm > 0.0)
        ? pow(max(dot(normal, halfwayDir), 0.0), max(Mat_KsStrenght, 1.0))
        : 0.0;

    vec3 color = Mat_Ka * ambientStrength
        + Mat_Kd * diffuseTerm * lightColor
        + Mat_Ks * specularTerm * lightColor;

    FragColor = vec4(color, 1.0);
}
