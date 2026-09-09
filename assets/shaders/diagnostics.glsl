#type vertex
#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in vec2 aUv;
uniform mat4 MVP;
void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}


#type fragment
#version 460 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0);
}
