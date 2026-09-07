#type vertex
#version 460 core

// Phoenix-style vertex layout for imported (Assimp) skinned meshes.
// Attribute names/order match Phoenix's Mesh::setupMesh vertex buffer layout.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBiTangent;
layout (location = 5) in uvec4 aBoneID;
layout (location = 6) in vec4 aBoneWeight;

const int MAX_BONES = 100;

uniform mat4 MVP;
uniform mat4 model;
// Phoenix uploads bone transforms to a uniform named "gBones" (see Model::Draw).
uniform mat4 gBones[MAX_BONES];

out vec3 vNormal;

void main() {
    mat4 boneTransform = gBones[aBoneID[0]] * aBoneWeight[0];
    boneTransform     += gBones[aBoneID[1]] * aBoneWeight[1];
    boneTransform     += gBones[aBoneID[2]] * aBoneWeight[2];
    boneTransform     += gBones[aBoneID[3]] * aBoneWeight[3];

    // Fall back to an unskinned pose if no bone weights were supplied
    // (e.g. static primitives that still use this shader).
    float totalWeight = aBoneWeight[0] + aBoneWeight[1] + aBoneWeight[2] + aBoneWeight[3];
    mat4 skinTransform = totalWeight > 0.0 ? boneTransform : mat4(1.0);

    vec4 skinnedPosition = skinTransform * vec4(aPos, 1.0);
    vNormal = mat3(model) * mat3(skinTransform) * aNormal;

    gl_Position = MVP * skinnedPosition;
}


#type fragment
#version 460 core

out vec4 FragColor;

in vec3 vNormal;

// This shader deliberately samples no textures: it is meant for animated models that either
// have no texture files (or none the user wants to use), so shading comes entirely from the
// per-mesh material colors Phoenix uploads (see Mesh::setMaterialShaderVars / FR-016).
uniform vec3 Mat_Ka;          // ambient color
uniform vec3 Mat_Kd = vec3(0.8, 0.8, 0.8);  // diffuse color
uniform vec3 Mat_Ks;          // specular color
uniform float Mat_KsStrenght; // specular strength/shininess
uniform vec3 uCameraPos;
uniform vec3 lightDirection = vec3(0.4, 0.8, 0.4);

void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(lightDirection);

    // Simple ambient + Lambertian diffuse + Blinn-Phong specular combination, driven purely by
    // the imported model's own material colors (no texture lookups at all).
    float diffuseTerm = max(dot(normal, lightDir), 0.0);

    vec3 viewDir = normalize(uCameraPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float specularTerm = Mat_KsStrenght > 0.0
        ? pow(max(dot(normal, halfwayDir), 0.0), max(Mat_KsStrenght, 1.0))
        : 0.0;

    vec3 color = Mat_Ka + Mat_Kd * diffuseTerm + Mat_Ks * specularTerm;
    FragColor = vec4(color, 1.0);
}
