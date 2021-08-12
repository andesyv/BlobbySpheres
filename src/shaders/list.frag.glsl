#version 450

layout(pixel_center_integer) in vec4 gl_FragCoord;

in vec4 gFragmentPosition;
flat in vec4 gSpherePosition;
flat in float gSphereRadius;
flat in float gOuterRadius;
flat in uint gSphereId;

uniform mat4 MVP;
uniform mat4 MVPInverse;

layout(binding = 0) uniform sampler2D positionTexture;
layout(r32ui, binding = 1) uniform uimage2D abufferIndexTexture;

struct FragmentEntry
{
	vec4 pos;
	uint prev;
};

layout(std430, binding = 0) buffer intersectionBuffer
{
	FragmentEntry intersections[LIST_MAX_ENTRIES];
};

layout(binding = 0) uniform atomic_uint counter;

struct Sphere
{			
	bool hit;
	vec3 near;
	vec3 far;
};
																					
Sphere calcSphereIntersection(float r, vec3 origin, vec3 center, vec3 line)
{
	vec3 oc = origin - center;
	vec3 l = normalize(line);
	float loc = dot(l, oc);
	float under_square_root = loc * loc - dot(oc, oc) + r*r;
	if (under_square_root > 0)
	{
		float da = -loc + sqrt(under_square_root);
		float ds = -loc - sqrt(under_square_root);
		vec3 near = origin+min(da, ds) * l;
		vec3 far = origin+max(da, ds) * l;

		return Sphere(true, near, far);
	}
	else
	{
		return Sphere(false, vec3(0), vec3(0));
	}
}

float calcDepth(vec3 pos)
{
	float far = gl_DepthRange.far; 
	float near = gl_DepthRange.near;
	vec4 clip_space_pos = MVP * vec4(pos, 1.0);
	float ndc_depth = clip_space_pos.z / clip_space_pos.w;
	return (((far - near) * ndc_depth) + near + far) / 2.0;
}

void main()
{
	vec4 fragCoord = gFragmentPosition;
	fragCoord /= fragCoord.w;
	
	vec4 near = MVPInverse*vec4(fragCoord.xy,-1.0,1.0);
	near /= near.w;

	vec4 far = MVPInverse*vec4(fragCoord.xy,1.0,1.0);
	far /= far.w;

	vec3 V = normalize(far.xyz-near.xyz);	
	Sphere sphere = calcSphereIntersection(gOuterRadius, near.xyz, gSpherePosition.xyz, V);
	
	if (!sphere.hit)
		discard;

	vec4 position = texelFetch(positionTexture,ivec2(gl_FragCoord.xy),0);
	
	float dist = length(sphere.near.xyz-near.xyz);
	
	if (dist > position.w)
		discard;

	uint index = atomicCounterIncrement(counter);
	if (LIST_MAX_ENTRIES <= index)
		discard;
	uint prev = imageAtomicExchange(abufferIndexTexture,ivec2(gl_FragCoord.xy),index);


	FragmentEntry entry;
	entry.pos = vec4(gSpherePosition.xyz, gSphereRadius);
	entry.prev = prev;

	intersections[index] = entry;

	discard;
}