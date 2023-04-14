#version 450

vec2 A = { -0.5,  0.5 }; // A--D
vec2 B = { -0.5, -0.5 }; // |\ |
vec2 C = {  0.5,  0.5 }; // | \|
vec2 D = {  0.5, -0.5 }; // B--C

vec2 positions[4] = { A, B, C, D};

vec3 colors[4] = {
    { 1.0, 0.0, 0.0 }, // A
    { 0.0, 1.0, 0.0 }, // B
    { 0.0, 1.0, 0.0 }, // C
    { 1.0, 0.0, 0.0 }, // D
};

vec2 texcoords[4] = {
    { 0.0, 0.0 }, // A
    { 0.0, 1.0 }, // B
    { 1.0, 1.0 }, // C
    { 1.0, 0.0 }, // D
};

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
