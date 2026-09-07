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
// Auto-supplied by ShaderEditor's playback clock (see spec 004); optional.
uniform float t;

out vec2 vUv;
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
    vUv = aTexCoords;

    gl_Position = MVP * skinnedPosition;
}


#type fragment
#version 460 core

out vec4 FragColor;

in vec2 vUv;
in vec3 vNormal;

// Phoenix's Material::loadTextures names the first diffuse texture slot "texture_diffuse1".
uniform sampler2D texture_diffuse1;
uniform vec3 lightDirection = vec3(0.4, 0.8, 0.4);

void main() {
    vec3 normal = normalize(vNormal);
    float diffuse = max(dot(normal, normalize(lightDirection)), 0.2);
    vec4 albedo = texture(texture_diffuse1, vUv);
    FragColor = vec4(albedo.rgb * diffuse, albedo.a);
}
