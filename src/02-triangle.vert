#version 450

layout(location = 0) out vec3 fragColor;

vec2 positions[] = {
    {  0.0, -0.5 },
    {  0.5,  0.5 },
    { -0.5,  0.5 },
};

vec3 colors[] = {
    { 1.0, 0.0, 0.0 },
    { 0.0, 1.0, 0.0 },
    { 0.0, 0.0, 1.0 },
};

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
