#version 450

layout(local_size_x = 2) in;

uniform mat4 MVP = mat4(1.0);

layout(binding = 0) uniform sampler3D volumeSample;
layout(binding = 1) uniform sampler3D volumeSample2;
layout(std430, binding = 2) buffer sceneBuffer
{
    vec4 lod0[];
};

layout(std430, binding = 3) buffer sceneBuffer2
{
    vec4 lod1[];
};

float sdfSphere(vec3 sp, float sr, vec3 p) {
    return length(sp - p) - sr;
}

float smin(float a, float b, float k)
{
    float h = max(k-abs(a-b), 0.0)/k;
    return min(a, b) - h*h*k*(1.0/4.0);
}

const bool FIRST_LOD = gl_LocalInvocationID.x == 0;

float sdf(vec3 p) {
    vec4 first = FIRST_LOD ? lod0[0] : lod1[0];
    float dist = sdfSphere(first.xyz, first.w, p);
    for (uint i = 1u; i < (FIRST_LOD ? SCENE_SIZE : SCENE_SIZE2); ++i)
        dist = smin(dist, sdfSphere((FIRST_LOD ? lod0 : lod1)[i].xyz, (FIRST_LOD ? lod0 : lod1)[i].w, p), 0.4);
    return dist;
}

void main() {
    vec3 ndc_coord = vec3(gl_WorkGroupID) / vec3(gl_NumWorkGroups - 1);
    vec4 p = MVP * vec4(ndc_coord, 1.0);
    p /= p.w;

    if (FIRST_LOD)
        imageStore(volumeSample, ivec3(gl_WorkGroupID), vec4(sdf(p.xyz)));
    else
        imageStore(volumeSample2, ivec3(gl_WorkGroupID), vec4(sdf(p.xyz)));
}