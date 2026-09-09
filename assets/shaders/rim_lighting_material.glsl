#type vertex
#version 460 core

// Same Phoenix-style skinned vertex layout as assets/shaders/mega_material.glsl, so the engine
// can upload the exact same per-mesh/material/bone/light uniforms without any shader-specific
// branching on the C++ side (spec 008, User Story 6).
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
uniform mat4 gBones[MAX_BONES];

out vec2 vUv;
out vec3 vNormal;
out vec3 vWorldPos;

void main() {
    mat4 boneTransform = gBones[aBoneID[0]] * aBoneWeight[0];
    boneTransform     += gBones[aBoneID[1]] * aBoneWeight[1];
    boneTransform     += gBones[aBoneID[2]] * aBoneWeight[2];
    boneTransform     += gBones[aBoneID[3]] * aBoneWeight[3];

    float totalWeight = aBoneWeight[0] + aBoneWeight[1] + aBoneWeight[2] + aBoneWeight[3];
    mat4 skinTransform = totalWeight > 0.0 ? boneTransform : mat4(1.0);
    mat3 normalTransform = mat3(model) * mat3(skinTransform);

    vec4 skinnedPosition = skinTransform * vec4(aPos, 1.0);
    vNormal = normalize(normalTransform * aNormal);
    vWorldPos = vec3(model * skinnedPosition);
    vUv = aTexCoords;

    gl_Position = MVP * skinnedPosition;
}


#type fragment
#version 460 core

// Rim lighting / Fresnel highlight (spec 008, User Story 6): adds a bright edge/halo along the
// object's silhouette (where the surface normal is near-perpendicular to the view direction),
// on top of standard per-pixel diffuse lighting. Reuses the same material/bone/single-light data
// as assets/shaders/mega_material.glsl.

out vec4 FragColor;

in vec2 vUv;
in vec3 vNormal;
in vec3 vWorldPos;

uniform sampler2D texture_diffuse1;
uniform bool hasDiffuseTexture = false;
uniform vec3 Mat_Kd = vec3(1.0);
uniform vec3 Mat_Ka = vec3(0.0);

uniform vec3 lightPosition = vec3(3.0, 4.0, 5.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 uCameraPos = vec3(0.0, 0.0, 4.0);
uniform float ambientStrength = 1.0;

// Color and intensity falloff of the rim highlight.
uniform vec3 rimColor = vec3(1.0, 1.0, 1.0);
uniform float rimPower = 2.0;

void main() {
    vec3 albedo = hasDiffuseTexture ? texture(texture_diffuse1, vUv).rgb : Mat_Kd;
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(lightPosition - vWorldPos);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);

    float diffuseTerm = max(dot(normal, lightDir), 0.0);
    vec3 baseColor = Mat_Ka * ambientStrength + albedo * diffuseTerm * lightColor;

    // Classic Fresnel-style rim term: near 0 when facing the camera, near 1 at grazing angles
    // (the object's silhouette edge).
    float rimFactor = pow(1.0 - max(dot(normal, viewDir), 0.0), max(rimPower, 0.001));
    vec3 color = baseColor + rimColor * rimFactor;

    FragColor = vec4(color, 1.0);
}
