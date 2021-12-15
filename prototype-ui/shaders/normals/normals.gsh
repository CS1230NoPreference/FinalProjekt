#version 330

layout(triangles) in;

layout(line_strip, max_vertices = 6) out;

// Transformation matrices
uniform mat4 p;
uniform mat4 v;
uniform mat4 m;

in vec2 tex[];
in vec4 normal[];

out vec2 texCoord;

const float NORMAL_LINE_LENGTH = 0.1; // Length of the normal arrow.

void main() {
    int i;
    for (i = 0; i < gl_in.length(); i++) {
        vec4 P = p * v * m * gl_in[i].gl_Position.xyzw;
        vec4 N = p * v * m * normal[i]; // p * vec4(normalize(mat3(transpose(inverse(v * m))) * normal[i].xyz), 0);
        vec2 T = tex[i];

        // Send the tex coordinates.
        texCoord = T;

        // Base coordinate.
        gl_Position = P;
        EmitVertex();

        // End of normal line.
        gl_Position = P + N * NORMAL_LINE_LENGTH;
        EmitVertex();

        EndPrimitive();
    }
}
