#pragma once
#include <linmath.h>

struct Vertex
{
	vec3 pos;
	vec3 normal;
	vec2 texcoord0;
};

struct Vertex* wavefront_read(_In_ FILE* file);