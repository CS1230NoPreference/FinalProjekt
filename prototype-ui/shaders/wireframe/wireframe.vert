#version 330 core

layout(location = 0) in vec3 in_position;
layout(location = 5) in vec2 in_texCoord;

// Transformation matrices
uniform mat4 p;
uniform mat4 v;
uniform mat4 m;

out vec2 texCoord;

void main() {
    texCoord = in_texCoord;
    gl_Position = p * v * m * vec4(in_position, 1.0);
}
