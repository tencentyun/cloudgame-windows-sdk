#version 440

layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
};

// RGBA 纹理
layout(binding = 1) uniform sampler2D rgbaTexture;

void main()
{
    // 直接采样RGBA纹理
    vec4 color = texture(rgbaTexture, qt_TexCoord0);
    
    // 应用透明度
    fragColor = color * qt_Opacity;
}