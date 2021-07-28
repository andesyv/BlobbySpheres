#version 450 core
layout (location = 0) in uint count;
layout (location = 1) in uvec2 screenCoord;
layout (location = 2) in vec4 pos[MAX_ENTRIES];

struct FragmentEntry
{
	uint count;
	uvec2 screenCoord;
	vec4 pos[MAX_ENTRIES];
};

out FragmentEntry vEntry;

void main()
{
    vec2 nCoords = (screenCoord / vec2(SCREEN_SIZE));
    nCoords.y = -nCoords.y; // screenCoords is top left while ndc is bottom left
    
    vEntry.count = count;
    vEntry.screenCoord = screenCoord;
    for (int i = 0; i < MAX_ENTRIES; ++i)
        vEntry.pos[i] = pos[i];

    gl_Position = vec4(nCoords, 0., 1.0);
}