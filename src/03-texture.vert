#version 450

const float Up = -1.0;
const float Dn =  1.0;
const float Lf = -1.0;
const float Rt =  1.0;

vec2 A = { Lf, Up }; // A--B
vec2 B = { Rt, Up }; // | /|
vec2 C = { Lf, Dn }; // |/ |
vec2 D = { Rt, Dn }; // C--D

vec2 positions[4] = { A, B, C, D };

vec3 colors[4] = {
    { 0.0, 0.0, 0.0 }, // A
    { 1.0, 0.0, 0.0 }, // B
    { 0.0, 1.0, 0.0 }, // C
    { 1.0, 1.0, 0.0 }, // D
};

vec2 texcoords[4] = {
    { 0.0, 0.0 }, // A
    { 1.0, 0.0 }, // B
    { 0.0, 1.0 }, // C
    { 1.0, 1.0 }, // D
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexcoord;

void main() {
    vec2 position = positions[gl_VertexIndex] * 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
    fragTexcoord = texcoords[gl_VertexIndex];
}
