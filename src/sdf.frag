#version 450 core

#define M_PI 3.14159
#define EPSILON 0.001

in vec2 uv;

uniform mat4 MVPInverse = mat4(1.0);
uniform float time = 0.0;

layout(std430, binding = 0) buffer sceneBuffer
{
	vec4 scene[];
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

float sdf(/*in vec4 scene[SCENE_SIZE], */vec3 p) {
    if (SCENE_SIZE < 1u)
        return -1.;
    else if (SCENE_SIZE < 2u)
        return sdfSphere(scene[0].xyz, scene[0].w, p);
    else {
        float m = sdfSphere(scene[0].xyz, scene[0].w, p);
        for (uint i = 1u; i < SCENE_SIZE; ++i)
            m = smin(m, sdfSphere(scene[i].xyz, scene[i].w, p), 0.13);
        return m;
    }
}

vec3 gradient(vec3 p) {
    return vec3(
        sdf(p + vec3(EPSILON, 0., 0.)) - sdf(p - vec3(EPSILON, 0., 0.)),
        sdf(p + vec3(0., EPSILON, 0.)) - sdf(p - vec3(0., EPSILON, 0.)),
        sdf(p + vec3(0., 0., EPSILON)) - sdf(p - vec3(0., 0., EPSILON))
    );
}

void main()
{
    // Near and far plane
    vec4 near = MVPInverse * vec4(uv, -1., 1.0);
    near /= near.w;
    vec4 far = MVPInverse * vec4(uv, 1., 1.0);
    far /= far.w;

    vec4 ro = vec4(near.xyz, 0.0);
    vec4 rd = vec4(normalize((far - near).xyz), 1.0);

    float t4 = time / 4.0;
    float t3 = time / 3.0;

    // vec4 scene[SCENE_SIZE] = vec4[SCENE_SIZE](
    //     vec4(0.1 + sin(time) * 0.1, 0.1, 0., 0.02),
    //     vec4(0.1, 0.2, 0.02, 0.06),
    //     vec4(0.0, sin(t4) * 0.1, cos(t4) * -0.3, 0.03),
    //     vec4(-0.3 * cos(t3), 0.0, 0.0, 0.06)
    // );

    // vec4 p = ro;
    // for (uint i = 0u; i < 100u; ++i) {
    //     float dist = sdf(p.xyz);

    //     if (1000.0 <= dist)
    //         break;

    //     if (dist < EPSILON) {
    //         vec3 grad = gradient(p.xyz);
    //         vec3 lightDir = rd.xyz;
    //         vec3 normal = normalize(grad);
    //         vec3 phong = vec3(1.0, 0.0, 0.0) * max(dot(normal, -lightDir), 0.15);
    //         fragColor = vec4(phong, 1.0);
    //         return;
    //     }

    //     p += rd * dist;
    // }

    fragColor = vec4(abs(rd.xyz) * 0.6, 1.0);
}