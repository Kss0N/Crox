#pragma once
#include <linmath.h>
#include <stdint.h>

struct Vertex
{
	vec3 pos;
	vec3 normal;
	vec2 texcoord0;
};

struct Vertex2
{
	float	 position[3];
	float	 normal[3];
	uint16_t texCoord[2];
	uint8_t	 color[4];
};