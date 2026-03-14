#version 450
#pragma shader_stage(fragment)

layout (location = 0) in vec2 fragCoord;
layout (location = 1) in vec4 fragColor;
layout(location = 0) out vec4 outColor;
layout(binding = 1) uniform sampler2D sampleTexture1;
layout(binding = 2) uniform sampler2D sampleTexture2;

void main() {
    outColor = texture(sampleTexture1, fragCoord) * texture(sampleTexture2, fragCoord);
    //outColor = texture(sampleTexture1, fragCoord) * fragColor;
}