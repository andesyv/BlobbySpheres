#version 450

#define EPSILON 0.001

layout(local_size_x = 1) in;

uniform mat4 MVPInverse = mat4(1.0);

layout(binding = 0, r16f) writeonly uniform image3D volumeDiff;
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

float sdf(vec3 p, bool first_lod) {
    vec4 first = first_lod ? lod0[0] : lod1[0];
    float dist = sdfSphere(first.xyz, first.w, p);
    for (uint i = 1u; i < (first_lod ? SCENE_SIZE : SCENE_SIZE2); ++i)
        dist = smin(dist, sdfSphere((first_lod ? lod0 : lod1)[i].xyz, (first_lod ? lod0 : lod1)[i].w, p), 0.4);
    return dist;
}

vec3 gradient(vec3 p, bool first_lod) {
    return vec3(
        sdf(p + vec3(EPSILON, 0., 0.), first_lod) - sdf(p - vec3(EPSILON, 0., 0.), first_lod),
        sdf(p + vec3(0., EPSILON, 0.), first_lod) - sdf(p - vec3(0., EPSILON, 0.), first_lod),
        sdf(p + vec3(0., 0., EPSILON), first_lod) - sdf(p - vec3(0., 0., EPSILON), first_lod)
    );
}

void main() {
    vec3 ndc_coord = 2.0 * (vec3(gl_WorkGroupID) / vec3(gl_NumWorkGroups - 1)) - 1.0;
    vec4 p = MVPInverse * vec4(ndc_coord, 1.0);
    p /= p.w;

//    vec3 g_lod0 = gradient(p.xyz, true);
//    vec3 g_lod1 = gradient(p.xyz, false);
    float lod0 = sdf(p.xyz, true);
    float lod1 = sdf(p.xyz, false);

//    imageStore(volumeDiff, ivec3(gl_WorkGroupID), vec4(g_lod1 - g_lod0, 0.));
    imageStore(volumeDiff, ivec3(gl_WorkGroupID), vec4(lod1 - lod0));
}