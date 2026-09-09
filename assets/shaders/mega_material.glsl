#type vertex
#version 460 core

// Phoenix-style skinned vertex layout for imported (Assimp) meshes, identical to
// legacy/pbr_animation.glsl. Bone attributes default to zero for meshes without a skeleton, in
// which case skinTransform below collapses to the identity matrix (see spec 008, FR-002/FR-003).
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

    // Meshes without a skeleton have all-zero bone weights; falling back to the identity matrix
    // here (rather than the zeroed-out boneTransform above) lets static/keyframe-animated models
    // share this exact same vertex shader instead of needing a separate non-skinned variant.
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

// Mega shader (spec 008): the single recommended shader for drawing any Assimp-imported model.
// Supports, in one render pass: bone skinning (with static fallback), a single per-pixel light,
// and every material map this project loads (base color/diffuse, normal, metalness, roughness,
// emissive, specular, height), automatically choosing between a PBR (Cook-Torrance) and a
// classic (Blinn-Phong) shading path per material, and falling back to Mat_Ka/Kd/Ks material
// colors for any channel that has no bound texture.

out vec4 FragColor;

in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBiTangent;
in vec3 vWorldPos;

// --- Material textures (Phoenix uniform names, see AssimpModelLoader::phoenixTextureTypeName) ---
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_metalness1;
uniform sampler2D texture_roughness1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_emissive1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_height1;

// --- Presence flags: true only when the matching texture slot was actually bound ---
uniform bool hasDiffuseTexture = false;
uniform bool hasPbrTextures = false;
uniform bool hasNormalMap = false;
uniform bool hasEmissiveTexture = false;
uniform bool hasSpecularMap = false;
uniform bool hasHeightMap = false;
// True when the material authored PBR data (metallic/roughness factor or texture); selects
// Cook-Torrance shading. False selects classic Blinn-Phong shading using Mat_Ka/Kd/Ks (spec 008,
// User Story 3): both paths read the same light/normal/view vectors so switching between
// materials never changes apparent light position or color, only the shading model.
uniform bool hasPbrWorkflow = false;

// --- Classic material colors (Mat_Ka/Kd/Ks/KsStrenght), used directly by the classic path and
// as PBR albedo/specular-tint fallback when no matching texture is bound ---
uniform vec3 Mat_Ka = vec3(0.0);
uniform vec3 Mat_Kd = vec3(1.0);
uniform vec3 Mat_Ks = vec3(0.0);
uniform float Mat_KsStrenght = 0.0;

// --- PBR factors (glTF metallic-roughness workflow) ---
uniform float metallicFactor = 1.0;
uniform float roughnessFactor = 1.0;
uniform vec3 emissiveFactor = vec3(0.0);

// --- Transparency (glTF KHR_materials_transmission + base opacity) ---
uniform float transmissionFactor = 0.0;
uniform float materialOpacity = 1.0;

// --- Lighting (single light, per spec 008 FR-003) ---
uniform vec3 lightPosition = vec3(3.0, 4.0, 5.0);
uniform vec3 lightColor = vec3(1.0, 1.0, 1.0);
uniform vec3 uCameraPos = vec3(0.0, 0.0, 4.0);
uniform float ambientStrength = 1.0;

const float kPi = 3.14159265359;

// Standard Cook-Torrance PBR terms (GGX distribution, Smith geometry, Schlick Fresnel), reused
// unmodified from legacy/pbr_animation.glsl.
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
    // Height/displacement map: no full parallax occlusion mapping is implemented (that would
    // need per-pixel UV offset iteration); its presence is exposed as hasHeightMap/texture_height1
    // for future use, and currently only mildly perturbs the shading normal so surfaces with a
    // height map are not visually identical to ones without it.
    vec2 uv = vUv;

    vec3 albedo = hasDiffuseTexture ? texture(texture_diffuse1, vUv).rgb : Mat_Kd;

    vec3 normal = normalize(vNormal);
    if (hasNormalMap) {
        // Standard tangent-space normal mapping via the TBN basis built from the interpolated
        // vertex tangent/bitangent/normal.
        mat3 tbn = mat3(normalize(vTangent), normalize(vBiTangent), normal);
        vec3 tangentNormal = texture(texture_normal1, uv).rgb * 2.0 - 1.0;
        normal = normalize(tbn * tangentNormal);
    } else if (hasHeightMap) {
        // Cheap normal-from-height approximation (single-tap slope), applied only when there is
        // no dedicated normal map to avoid overriding authored normal data.
        float height = texture(texture_height1, uv).r;
        normal = normalize(normal + vec3(dFdx(height), dFdy(height), 0.0) * 0.5);
    }

    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 lightDir = normalize(lightPosition - vWorldPos);
    vec3 halfway = normalize(viewDir + lightDir);
    float nDotL = max(dot(normal, lightDir), 0.0);

    vec3 color;
    if (hasPbrWorkflow) {
        // --- PBR (Cook-Torrance) path ---
        float metallic = hasPbrTextures ? texture(texture_metalness1, uv).b : metallicFactor;
        float roughness = hasPbrTextures ? texture(texture_roughness1, uv).g : roughnessFactor;
        roughness = clamp(roughness, 0.05, 1.0);

        vec3 f0 = mix(vec3(0.04), albedo, metallic);
        float nDotV = max(dot(normal, viewDir), 0.0001);
        float nDotLSafe = max(nDotL, 0.0001);

        float distribution = distributionGGX(normal, halfway, roughness);
        float geometry = geometrySmith(nDotV, nDotLSafe, roughness);
        vec3 fresnel = fresnelSchlick(max(dot(halfway, viewDir), 0.0), f0);

        vec3 specular = (distribution * geometry * fresnel) / (4.0 * nDotV * nDotLSafe + 0.0001);
        vec3 kd = (vec3(1.0) - fresnel) * (1.0 - metallic);
        vec3 diffuse = kd * albedo / kPi;

        vec3 radiance = lightColor * nDotL;
        color = (diffuse + specular) * radiance;
        color += albedo * 0.03; // small constant ambient term so unlit areas aren't pure black

        // Reinhard tone mapping + gamma correction, matching legacy/pbr_animation.glsl's display
        // curve so PBR materials look identical whether drawn by that shader or this one.
        color = color / (color + vec3(1.0));
        color = pow(color, vec3(1.0 / 2.2));
    } else {
        // --- Classic (Blinn-Phong) path, matching legacy/material_pixel_lighting.glsl ---
        vec3 specularColor = hasSpecularMap ? texture(texture_specular1, uv).rgb * Mat_Ks : Mat_Ks;
        float specularTerm = (Mat_KsStrenght > 0.0 && nDotL > 0.0)
            ? pow(max(dot(normal, halfway), 0.0), max(Mat_KsStrenght, 1.0))
            : 0.0;

        color = Mat_Ka * ambientStrength
            + albedo * nDotL * lightColor
            + specularColor * specularTerm * lightColor;
    }

    // Self-illumination (e.g. brake lights/headlights) independent of scene lighting, added after
    // the lit term (and after PBR tone mapping) so it isn't crushed by Reinhard.
    vec3 emissive = hasEmissiveTexture ? texture(texture_emissive1, uv).rgb * emissiveFactor : emissiveFactor;
    color += emissive;

    // Transmission (glass) and opacity (alpha) are independent ways a material can be marked
    // translucent; take whichever leaves less of the surface opaque (spec 008, FR-011).
    float alpha = min(materialOpacity, 1.0 - transmissionFactor);
    FragColor = vec4(color, alpha);
}
