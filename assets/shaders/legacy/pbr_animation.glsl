#type vertex
#version 460 core

// Same Phoenix-style skinned vertex layout as assets/shaders/bone_animation.glsl,
// combined here with a PBR (metallic-roughness) fragment stage.
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

out vec2 vUv;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBiTangent;
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
    vTangent = normalize(normalTransform * aTangent);
    vBiTangent = normalize(normalTransform * aBiTangent);
    vWorldPos = vec3(model * skinnedPosition);
    vUv = aTexCoords;

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

// Phoenix's Material::loadTextures names these slots "texture_diffuse1"
// (glTF baseColorTexture), "texture_metalness1" and "texture_roughness1"
// (glTF metallicRoughnessTexture channels split per aiTextureType).
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_metalness1;
uniform sampler2D texture_roughness1;
// glTF normalTexture (tangent-space normal map). Applied via the TBN basis built from the
// interpolated vertex tangent/bitangent/normal; falls back to the plain vertex normal when no
// normal map is bound (hasNormalMap is false).
uniform sampler2D texture_normal1;
uniform bool hasNormalMap = false;
// glTF emissiveTexture, combined (multiplied) with emissiveFactor below for self-illumination
// that doesn't depend on scene lighting (e.g. brake lights, headlights, glowing panels).
uniform sampler2D texture_emissive1;
uniform bool hasEmissiveTexture = false;
uniform vec3 emissiveFactor = vec3(0.0);
// Set to 1 when dedicated metalness/roughness textures are bound; falls back
// to the scalar factors below otherwise (e.g. for built-in primitives).
uniform bool hasPbrTextures = false;
// Set to 1 when texture_diffuse1 is actually bound. Some PBR materials (e.g. glTF's
// baseColorFactor-only materials) have no base color texture at all; sampling an unbound
// sampler would return garbage/black, so Mat_Kd is used as the albedo in that case instead.
uniform bool hasDiffuseTexture = false;
uniform vec3 Mat_Kd = vec3(1.0);
uniform float metallicFactor = 1.0;
uniform float roughnessFactor = 1.0;
// glTF's KHR_materials_transmission factor and base alpha (see ModelMaterial::transmissionFactor/
// opacity); together they drive this fragment's output alpha so glass-like materials (e.g. this
// car's windshield) render translucent instead of always fully opaque.
uniform float transmissionFactor = 0.0;
uniform float materialOpacity = 1.0;

uniform vec3 lightPosition = vec3(3.0, 4.0, 5.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 uCameraPos = vec3(0.0, 0.0, 4.0);

const float kPi = 3.14159265359;

// Standard Cook-Torrance PBR terms (GGX distribution, Smith geometry, Schlick Fresnel).
float distributionGGX(vec3 normal, vec3 halfway, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float nDotH = max(dot(normal, halfway), 0.0);
    float denom = (nDotH * nDotH) * (a2 - 1.0) + 1.0;
    return a2 / (kPi * denom * denom);
}

float geometrySmith(float nDotV, float nDotL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float ggxV = nDotV / (nDotV * (1.0 - k) + k);
    float ggxL = nDotL / (nDotL * (1.0 - k) + k);
    return ggxV * ggxL;
}

vec3 fresnelSchlick(float cosTheta, vec3 f0) {
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 albedo = hasDiffuseTexture ? texture(texture_diffuse1, vUv).rgb : Mat_Kd;
    // glTF packs the combined metallic-roughness (and often occlusion) texture with roughness in
    // the green channel and metalness in the blue channel; Assimp hands back the same source
    // image for both texture_metalness1 and texture_roughness1 (it does not split channels), so
    // each slot must be sampled from its own channel here rather than both reading .r (which is
    // occlusion, not metal/roughness, and was why every material looked equally flat before).
    float metallic = hasPbrTextures ? texture(texture_metalness1, vUv).b : metallicFactor;
    float roughness = hasPbrTextures ? texture(texture_roughness1, vUv).g : roughnessFactor;
    roughness = clamp(roughness, 0.05, 1.0);

    vec3 normal = normalize(vNormal);
    if (hasNormalMap) {
        // Standard tangent-space normal mapping: sample, unpack from [0,1] to [-1,1], then
        // rotate into world space via the TBN basis built from the interpolated vertex vectors.
        mat3 tbn = mat3(normalize(vTangent), normalize(vBiTangent), normal);
        vec3 tangentNormal = texture(texture_normal1, vUv).rgb * 2.0 - 1.0;
        normal = normalize(tbn * tangentNormal);
    }
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 lightDir = normalize(lightPosition - vWorldPos);
    vec3 halfway = normalize(viewDir + lightDir);

    vec3 f0 = mix(vec3(0.04), albedo, metallic);
    float nDotV = max(dot(normal, viewDir), 0.0001);
    float nDotL = max(dot(normal, lightDir), 0.0001);

    float distribution = distributionGGX(normal, halfway, roughness);
    float geometry = geometrySmith(nDotV, nDotL, roughness);
    vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDir), 0.0), f0);

    vec3 specular = (distribution * geometry * fresnel) / (4.0 * nDotV * nDotL + 0.0001);
    vec3 kd = (vec3(1.0) - fresnel) * (1.0 - metallic);
    vec3 diffuse = kd * albedo / kPi;

    vec3 radiance = lightColor * nDotL;
    vec3 color = (diffuse + specular) * radiance;
    color += albedo * 0.03; // small constant ambient term so unlit areas aren't pure black

    // Self-illumination (e.g. brake lights/headlights) that doesn't depend on scene lighting;
    // added after the lit term, before tone mapping, so it isn't crushed by Reinhard the same
    // way an equivalent boost to the light-dependent color would be.
    vec3 emissive = hasEmissiveTexture ? texture(texture_emissive1, vUv).rgb * emissiveFactor : emissiveFactor;
    color += emissive;

    // Reinhard tone mapping + gamma correction for display.
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    // Transmission (glass) and opacity (alpha) are independent ways glTF marks a material
    // translucent; take whichever leaves less of the surface opaque.
    float alpha = min(materialOpacity, 1.0 - transmissionFactor);
    FragColor = vec4(color, alpha);
}
