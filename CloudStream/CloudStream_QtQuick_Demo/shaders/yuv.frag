#version 450 core

layout(location = 0) in vec2 v_texCoord;
layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D texY;
layout(binding = 2) uniform sampler2D texU;
layout(binding = 3) uniform sampler2D texV;

layout(std140, binding = 0) uniform qt_Matrix {
    mat4 matrix;
};

void main()
{
    float y = texture(texY, v_texCoord).r;
    float u = texture(texU, v_texCoord).r - 0.5;
    float v = texture(texV, v_texCoord).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;

    outColor = vec4(r, g, b, 1.0);
}