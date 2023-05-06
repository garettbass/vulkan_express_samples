#version 460

struct Instance {
    vec2 origin;
    vec2 extent;
};

layout(std140,set=0,binding=3)
buffer readonly _0_3 {
    Instance instances[];
};

const float Lf = -1.0;
const float Up = -1.0;
const float Rt =  1.0;
const float Dn =  1.0;

vec2 A = { Lf, Up }; // A--B
vec2 B = { Rt, Up }; // | /|
vec2 C = { Lf, Dn }; // |/ |
vec2 D = { Rt, Dn }; // C--D

vec2 positions[] = { A, B, C, D };

vec3 colors[] = {
    { 0.0, 0.0, 0.0 }, // A
    { 1.0, 0.0, 0.0 }, // B
    { 0.0, 1.0, 0.0 }, // C
    { 1.0, 1.0, 0.0 }, // D
};

vec2 texcoords[] = {
    { 0.0, 0.0 }, // A
    { 1.0, 0.0 }, // B
    { 0.0, 1.0 }, // C
    { 1.0, 1.0 }, // D
};

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexcoord;

void main() {
    Instance instance = instances[gl_InstanceIndex];
    vec2 position = positions[gl_VertexIndex];
    position = position * instance.extent;
    position = position + instance.origin;
    gl_Position = vec4(position, 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
    fragTexcoord = texcoords[gl_VertexIndex];
}
