#version 450 core

#define M_PI 3.14159
#define EPSILON 0.001
#define MAX_STEPS 100u

layout(origin_upper_left, pixel_center_integer) in vec4 gl_FragCoord;

in vec2 ndc;

uniform mat4 MVPInverse = mat4(1.0);
uniform float time = 0.0;

layout(location = 0) uniform sampler2D positionTex; 

struct FragmentEntry
{
	vec4 pos[MAX_ENTRIES];
	uint count;
};

layout(std430, binding = 0) buffer intersectionBuffer
{
	FragmentEntry intersections[];
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

float sdf(in vec4 entries[MAX_ENTRIES], uint entryCount, vec3 p) {
    if (entryCount < 1u)
        return -1.;
    else if (entryCount < 2u)
        return sdfSphere(entries[0].xyz, entries[0].w, p);
    else {
        float m = sdfSphere(entries[0].xyz, entries[0].w, p);
        for (uint i = 1u; i < entryCount; ++i)
            // m = smin(m, sdfSphere(entries[i].xyz, entries[i].w, p), 0.13);
            m = min(m, sdfSphere(entries[i].xyz, entries[i].w, p));
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

    uint intersectionIndex = uint(gl_FragCoord.x) + uint(gl_FragCoord.y) * SCREEN_SIZE.x;

    uint entryCount = min(intersections[intersectionIndex].count, MAX_ENTRIES);

    if (entryCount != 0) {
        vec4 entries[MAX_ENTRIES];
        for (int i = 0; i < entryCount; ++i)
            entries[i] = intersections[intersectionIndex].pos[i];

        vec4 p = ro;
        for (uint i = 0u; i < MAX_STEPS; ++i) {
            float dist = sdf(entries, entryCount, p.xyz);

            if (1000.0 <= dist)
                break;

            if (dist < EPSILON) {
                vec3 grad = gradient(entries, entryCount, p.xyz);
                vec3 lightDir = rd.xyz;
                vec3 normal = normalize(grad);
                vec3 phong = vec3(1.0, 0.0, 0.0) * max(dot(normal, -lightDir), 0.15);
                fragColor = vec4(phong, 1.0);
                // fragColor = vec4(vec3(p.w + dist), 1.0);
                return;
            }

            p += rd * dist;
        }
    }

    fragColor = vec4(abs(rd.xyz) * 0.6, 1.0);
}