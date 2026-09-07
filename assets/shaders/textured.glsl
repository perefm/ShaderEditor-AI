#type vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUv;
uniform mat4 MVP;
out vec2 vUv;
void main() {
    vUv = aUv;
    gl_Position = MVP * vec4(aPos, 1.0);
}


#type fragment
#version 460 core
out vec4 FragColor;
uniform sampler2D imageTexture;
in vec2 vUv;
void main() {
    FragColor = texture(imageTexture, vUv);
}
