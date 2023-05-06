#version 460

#define uniform(SET, BINDING) uniform layout(set=SET,binding=BINDING)

uniform(0, 0) texture2D texcoordsRGBA;
uniform(0, 1) sampler   LINEAR_REPEAT;
uniform(0, 2) sampler   NEAREST_CLAMP;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexcoord;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(sampler2D(texcoordsRGBA, LINEAR_REPEAT), fragTexcoord);
}
