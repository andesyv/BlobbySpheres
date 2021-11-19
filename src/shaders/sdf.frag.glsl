#version 450 core

#define M_PI 3.14159
#define EPSILON 0.001
#define MAX_STEPS 100u

in vec2 ndc;

uniform mat4 MVPInverse = mat4(1.0);

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

//// power smooth min (k = 8);
//float p_smin( float a, float b, float k )
//{
//    a = pow( a, k ); b = pow( b, k );
//    return pow( (a*b)/(a+b), 1.0/k );
//}

float sdf(vec3 p) {
    float dist = sdfSphere(scene[0].xyz, scene[0].w, p);
    for (uint i = 1u; i < SCENE_SIZE; ++i)
        dist = smin(dist, sdfSphere(scene[i].xyz, scene[i].w, p), 0.4);
//        dist = min(dist, sdfSphere(scene[i].xyz, scene[i].w, p));
    return dist;
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
    vec4 near = MVPInverse * vec4(ndc, -1., 1.0);
    near /= near.w;
    vec4 far = MVPInverse * vec4(ndc, 1., 1.0);
    far /= far.w;

    vec4 ro = vec4(near.xyz, 0.0);
    vec4 rd = vec4(normalize((far - near).xyz), 1.0);

    vec4 p = ro;
    for (uint i = 0u; i < MAX_STEPS; ++i) {
        float dist = sdf(p.xyz);

        if (1000.0 <= dist)
            break;

        if (dist < EPSILON) {
            vec3 grad = gradient(p.xyz);
            vec3 lightDir = rd.xyz;
            vec3 normal = normalize(grad);
            vec3 phong = vec3(1.0, 0.0, 0.0) * max(dot(normal, -lightDir), 0.15);
            fragColor = vec4(phong, 1.0);
            return;
        }

        p += rd * dist;
    }

    fragColor = vec4(abs(rd.xyz) * 0.6, 1.0);
}