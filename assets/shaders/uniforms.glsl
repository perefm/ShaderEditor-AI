#type vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUv;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}


#type fragment
#version 460 core
out vec4 FragColor;
uniform float intensity;
uniform vec4 tint;
void main() {
    FragColor = tint * intensity;
}
