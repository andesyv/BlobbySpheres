#version 450 core

#define M_PI 3.14159
#define EPSILON 0.001
#define MAX_STEPS 100u

layout(pixel_center_integer) in vec4 gl_FragCoord;

in vec2 ndc;

uniform mat4 MVPInverse = mat4(1.0);
uniform float time = 0.0;
uniform float smoothing = 0.13;
uniform float interpolation = 0.0;

// layout(binding = 0) uniform sampler2D positionTex;
layout(binding = 1) uniform usampler2D abufferIndexTexture;
// layout(binding = 2) uniform sampler2D positionTex2;
layout(binding = 3) uniform usampler2D abufferIndexTexture2;

layout(std430, binding = 0) buffer intersectionBuffer
{
	vec4 intersections[];
};

out vec4 fragColor;

// SDF:
float sdfSphere(vec3 sp, float sr, vec3 p) {
    return length(sp - p) - sr;
}

// https://iquilezles.org/www/articles/smin/smin.htm
// polynomial smooth min (k = 0.1);
float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*k*(1.0/4.0);
}

// power smooth min (k = 8);
float p_smin( float a, float b, float k )
{
    a = pow( a, k ); b = pow( b, k );
    return pow( (a*b)/(a+b), 1.0/k );
}

float sdf(in vec4 entries[MAX_ENTRIES], uint entryCount, vec3 p) {
    if (entryCount < 1u)
        return -1.;
    else if (entryCount < 2u)
        return sdfSphere(entries[0].xyz, entries[0].w, p);
    else {
        float m = sdfSphere(entries[0].xyz, entries[0].w, p);
        for (uint i = 1u; i < entryCount; ++i)
            m = smin(m, sdfSphere(entries[i].xyz, entries[i].w, p), smoothing);
            // m = min(m, sdfSphere(entries[i].xyz, entries[i].w, p));
        return m;
    }
}

vec3 gradient(in vec4 entries[MAX_ENTRIES], uint entryCount, vec3 p) {
    return vec3(
        sdf(entries, entryCount, p + vec3(EPSILON, 0., 0.)) - sdf(entries, entryCount, p - vec3(EPSILON, 0., 0.)),
        sdf(entries, entryCount, p + vec3(0., EPSILON, 0.)) - sdf(entries, entryCount, p - vec3(0., EPSILON, 0.)),
        sdf(entries, entryCount, p + vec3(0., 0., EPSILON)) - sdf(entries, entryCount, p - vec3(0., 0., EPSILON))
    );
}

void main()
{
    // Near and far plane
    vec4 near = MVPInverse * vec4(ndc, -1., 1.0);
    near /= near.w;
    vec4 far = MVPInverse * vec4(ndc, 1., 1.0);
    far /= far.w;

    vec4 ro = vec4(near.xyz, 0.0);
    vec4 rd = vec4(normalize((far - near).xyz), 1.0);

    float t4 = time / 4.0;
    float t3 = time / 3.0;

    // Build index list:
    const uint bufferIndex = 2 * MAX_ENTRIES * (SCREEN_SIZE.y * uint(gl_FragCoord.x) + uint(gl_FragCoord.y));
    uint entryCount = min(texelFetch(abufferIndexTexture,ivec2(gl_FragCoord.xy),0).x, MAX_ENTRIES);
    uint entryCount2 = min(texelFetch(abufferIndexTexture2,ivec2(gl_FragCoord.xy),0).x, MAX_ENTRIES);
    bool bEmpty = entryCount == 0;
    bool bEmpty2 = entryCount2 == 0;
    if (bEmpty && bEmpty2) {
        fragColor = vec4(abs(rd.xyz) * 0.6, 1.0);
        return;
    }

    vec4 entries[MAX_ENTRIES], entries2[MAX_ENTRIES];
    if (!bEmpty)
        for (int i = 0; i < entryCount; ++i)
            entries[i] = intersections[bufferIndex + i];
    if (!bEmpty2)
        for (int i = 0; i < entryCount2; ++i)
            entries2[i] = intersections[bufferIndex + i + MAX_ENTRIES];

    vec4 p = ro;
    for (uint i = 0u; i < MAX_STEPS; ++i) {
        float dist =    bEmpty ? sdf(entries2, entryCount2, p.xyz) : 
                        bEmpty2 ? sdf(entries, entryCount, p.xyz) :
                        mix(sdf(entries, entryCount, p.xyz), sdf(entries2, entryCount2, p.xyz), interpolation);

        if (1000.0 <= dist)
            break;

        if (dist < EPSILON) {
            vec3 grad = bEmpty ? gradient(entries2, entryCount2, p.xyz) :
                        bEmpty2 ? gradient(entries, entryCount, p.xyz) :
                        mix(gradient(entries, entryCount, p.xyz), gradient(entries2, entryCount2, p.xyz), interpolation);
            vec3 lightDir = rd.xyz;
            vec3 normal = normalize(grad);
            vec3 phong = vec3(1.0, 0.0, 0.0) * max(dot(normal, -lightDir), 0.15);
            fragColor = vec4(phong, 1.0);
            // fragColor = vec4(vec3(p.w + dist), 1.0);
            return;
        }

        p += rd * dist;
    }

    fragColor = vec4(abs(rd.xyz) * 0.6, 1.0);
}