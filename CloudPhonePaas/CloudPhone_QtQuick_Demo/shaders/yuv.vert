#version 450 core

// Qt Quick Shader Binary默认使用的uniform block，包含变换矩阵
layout(std140, binding = 0) uniform qt_Matrix {
    mat4 matrix;
};

layout(location = 0) in vec2 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;
layout(location = 0) out vec2 v_texCoord;

void main()
{
    gl_Position = matrix * vec4(vertexPosition, 0.0, 1.0);
    v_texCoord = vertexTexCoord;
}