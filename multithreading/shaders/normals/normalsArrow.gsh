#version 330

layout(triangles) in;

layout(triangle_strip, max_vertices = 18) out;

// Transformation matrices
uniform mat4 p;
uniform mat4 v;
uniform mat4 m;

in vec2 tex[];
in vec4 normal[];

const float NORMAL_LINE_LENGTH = 0.1; // Length of the normal arrow.
const float NORMAL_ARROW_LENGTH = 0.05; // Length of the arrow head.
const float NORMAL_ARROW_WIDTH = 0.0006; // Width of the arrow head.

void main() {
    int i;
    for (i = 0; i < gl_in.length(); i++) {
        vec4 P = p * v * m * gl_in[i].gl_Position.xyzw;
        vec4 N = p * v * m * normal[i]; // p * vec4(normalize(mat3(transpose(inverse(v * m))) * normal[i].xyz), 0);

        // Base coordinate.
        gl_Position = P + N * (NORMAL_LINE_LENGTH + NORMAL_ARROW_LENGTH);
        EmitVertex();

        // Calculate perpendicular axis to offset the points on.
        vec4 eye = P + N * NORMAL_LINE_LENGTH;
        vec4 axis = normalize(vec4(cross(eye.xyz, N.xyz), 0.0));
        axis.z = 0.f;

        // Second point.
        gl_Position = P + N * NORMAL_LINE_LENGTH + axis * NORMAL_ARROW_WIDTH;
        EmitVertex();

        // Third point.
        gl_Position = P + N * NORMAL_LINE_LENGTH - axis * NORMAL_ARROW_WIDTH;
        EmitVertex();

        EndPrimitive();
    }
}
