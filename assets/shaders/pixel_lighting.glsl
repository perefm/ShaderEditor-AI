#type vertex
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aUv;

uniform mat4 MVP;

out vec3 vPosition;
out vec2 vUv;

void main() {
    vPosition = aPos;
    vUv = aUv;
    gl_Position = MVP * vec4(aPos, 1.0);
}


#type fragment
#version 460 core

out vec4 FragColor;

in vec3 vPosition;
in vec2 vUv;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform vec3 ambientColor;
uniform vec3 uCameraPos;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;
uniform float shininess;

void main() {
    vec3 normal = normalize(cross(dFdx(vPosition), dFdy(vPosition)));
    vec3 lightDirection = normalize(lightPosition - vPosition);
    vec3 viewDirection = normalize(uCameraPos - vPosition);
    vec3 halfwayDirection = normalize(lightDirection + viewDirection);

    float diffuse = max(dot(normal, lightDirection), 0.0);
    float specular = pow(max(dot(normal, halfwayDirection), 0.0), shininess);
    vec3 lighting = ambientColor * ambientStrength
        + lightColor * (diffuse * diffuseStrength + specular * specularStrength);

    FragColor = vec4(objectColor * lighting, 1.0);
}
