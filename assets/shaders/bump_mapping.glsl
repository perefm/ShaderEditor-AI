#type vertex
#version 460 core

// Phoenix-style vertex layout (also matches assets/shaders/bone_animation.glsl)
// so bump-mapped models can share the same imported mesh attributes.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBiTangent;

uniform mat4 MVP;
uniform mat4 model;

out vec2 vUv;
out vec3 vNormal;
out vec3 vTangent;
out vec3 vBiTangent;
out vec3 vWorldPos;

void main() {
    vUv = aTexCoords;
    vNormal = normalize(mat3(model) * aNormal);
    vTangent = normalize(mat3(model) * aTangent);
    vBiTangent = normalize(mat3(model) * aBiTangent);
    vWorldPos = vec3(model * vec4(aPos, 1.0));

    gl_Position = MVP * vec4(aPos, 1.0);
}


#type fragment
#version 460 core

out vec4 FragColor;

in vec2 vUv;
in vec3 vNormal;
in vec3 vTangent;
in vec3 vBiTangent;
in vec3 vWorldPos;

// Phoenix's Material::loadTextures names diffuse/normal map slots
// "texture_diffuse1" / "texture_normal1" (see Material.cpp::loadTextures).
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_normal1;
// Set to 1 when a real tangent-space normal map is bound; 0 falls back to a
// screen-space derivative bump so this shader still shows relief on models
// (or built-in primitives) that only carry a diffuse texture.
uniform bool hasNormalMap = false;

uniform vec3 lightPosition = vec3(3.0, 4.0, 5.0);
uniform vec3 uCameraPos = vec3(0.0, 0.0, 4.0);
uniform float bumpStrength = 1.0;
uniform float shininess = 32.0;

void main() {
    vec4 albedo = texture(texture_diffuse1, vUv);

    vec3 normal;
    if (hasNormalMap) {
        mat3 tbn = mat3(normalize(vTangent), normalize(vBiTangent), normalize(vNormal));
        vec3 sampledNormal = texture(texture_normal1, vUv).rgb * 2.0 - 1.0;
        sampledNormal.xy *= bumpStrength;
        normal = normalize(tbn * sampledNormal);
    } else {
        // Derive a fake bump from the diffuse texture's luminance gradient so the
        // effect is still visible without a dedicated normal map asset.
        float height = dot(albedo.rgb, vec3(0.299, 0.587, 0.114));
        float heightDx = dFdx(height);
        float heightDy = dFdy(height);
        vec3 bumped = normalize(vNormal) - bumpStrength * (heightDx * normalize(vTangent) + heightDy * normalize(vBiTangent));
        normal = normalize(bumped);
    }

    vec3 lightDir = normalize(lightPosition - vWorldPos);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float diffuseTerm = max(dot(normal, lightDir), 0.0);
    float specularTerm = pow(max(dot(normal, halfwayDir), 0.0), shininess);

    vec3 color = albedo.rgb * (0.15 + 0.85 * diffuseTerm) + vec3(specularTerm) * 0.35;
    FragColor = vec4(color, albedo.a);
}
