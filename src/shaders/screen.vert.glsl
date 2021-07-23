#version 450 core
layout (location = 0) in vec3 aPos;

out vec2 ndc;

void main()
{
    ndc = aPos.xy;
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}