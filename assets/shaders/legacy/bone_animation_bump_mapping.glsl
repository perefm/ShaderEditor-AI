#type vertex
#version 460 core

// Phoenix-compatible skinned vertex layout with tangent-space bump mapping data.
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
out vec3 vTangent;
out vec3 vBiTangent;
out vec3 vWorldPos;

void main() {
    mat4 boneTransform = gBones[aBoneID[0]] * aBoneWeight[0];
    boneTransform += gBones[aBoneID[1]] * aBoneWeight[1];
    boneTransform += gBones[aBoneID[2]] * aBoneWeight[2];
    boneTransform += gBones[aBoneID[3]] * aBoneWeight[3];

    float totalWeight = aBoneWeight[0] + aBoneWeight[1] + aBoneWeight[2] + aBoneWeight[3];
    mat4 skinTransform = totalWeight > 0.0 ? boneTransform : mat4(1.0);
    mat3 normalTransform = mat3(model) * mat3(skinTransform);
    vec4 skinnedPosition = skinTransform * vec4(aPos, 1.0);

    vUv = aTexCoords;
    vNormal = normalize(normalTransform * aNormal);
    vTangent = normalize(normalTransform * aTangent);
    vBiTangent = normalize(normalTransform * aBiTangent);
    vWorldPos = vec3(model * skinnedPosition);
    gl_Position = MVP * skinnedPosition;
}

#type fragment
#version 460 core

out vec4 FragColor;

in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBiTangent;
in vec3 vWorldPos;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
uniform bool hasNormalMap = false;
uniform vec3 lightPosition = vec3(3.0, 4.0, 5.0);
uniform vec3 uCameraPos = vec3(0.0, 0.0, 4.0);
uniform float bumpStrength = 1.0;
uniform float shininess = 32.0;

void main() {
    vec4 albedo = texture(texture_diffuse1, vUv);
    vec3 normal = normalize(vNormal);

    if (hasNormalMap) {
        mat3 tbn = mat3(normalize(vTangent), normalize(vBiTangent), normal);
        vec3 tangentNormal = texture(texture_normal1, vUv).rgb * 2.0 - 1.0;
        tangentNormal.xy *= bumpStrength;
        normal = normalize(tbn * tangentNormal);
    }

    vec3 lightDir = normalize(lightPosition - vWorldPos);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float diffuse = max(dot(normal, lightDir), 0.0);
    float specular = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    FragColor = vec4(albedo.rgb * (0.15 + 0.85 * diffuse) + vec3(specular) * 0.35, albedo.a);
}
