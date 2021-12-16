#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 5) in vec2 in_texCoord;

out vec2 tex;
out vec4 normal;

void main() {
    tex = in_texCoord;
    gl_Position = vec4(in_position, 1.0);
    normal = vec4(in_normal, 0.0);
}
