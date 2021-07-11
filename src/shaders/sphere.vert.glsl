#version 330 core

layout (location = 0) in vec4 inPos;

out float vRadius;

void main()
{
    vRadius = inPos.w;
    gl_Position = vec4(inPos.xyz, 1.0);
}