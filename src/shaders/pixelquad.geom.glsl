#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

struct FragmentEntry
{
	uint count;
	uvec2 screenCoord;
	vec4 pos[MAX_ENTRIES];
};

in FragmentEntry vEntry[];

out vec2 ndc;
flat out FragmentEntry gEntry;

void main() {
    gEntry = vEntry[0];
    ndc = gl_in[0].gl_Position.xy;
    // const vec2 texelHalfSize = vec2(10.0) / vec2(SCREEN_SIZE);
    const vec2 texelHalfSize = vec2(0.1);
    
    gl_Position = gl_in[0].gl_Position + vec4(-texelHalfSize.x, texelHalfSize.y, 0., 0.);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(-texelHalfSize.x, -texelHalfSize.y, 0., 0.);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(texelHalfSize.x, texelHalfSize.y, 0., 0.);
    EmitVertex();

    gl_Position = gl_in[0].gl_Position + vec4(texelHalfSize.x, -texelHalfSize.y, 0., 0.);
    EmitVertex();

    // gl_Position = vec4(-1, 1., 0., 0.0) + gl_in[0].gl_Position;
    // EmitVertex();

    // gl_Position = vec4(-1, -1., 0., 0.0) + gl_in[0].gl_Position;
    // EmitVertex();

    // gl_Position = vec4(1, 1., 0., 0.0) + gl_in[0].gl_Position;
    // EmitVertex();

    // gl_Position = vec4(1, -1., 0., 0.0) + gl_in[0].gl_Position;
    // EmitVertex();

    EndPrimitive();
}