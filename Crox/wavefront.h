#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <linmath.h>
#include <stb_image.h>
#include <stb_ds.h>

#include "mesh.h"
#include "NullSemantics.h"


typedef float* stbi_bmp;

struct WavefrontMesh {
	struct Vertex* vertices;	// stb_ds array
	uint32_t* indices;			// stb_ds array

	const char* name;		
	const char* mtlName;
};
enum WavefrontIlluminationModel {
	ColorNoAmbient,
	ColorAmbient,
	Highlight,
	ReflectionRaytrace,
	GlassRaytrace,
	FresnelRaytrace,
	RefractionRaytraceNoFresnel,
	RefractionFresnelRaytrace,
	ReflectionNoRaytrace,
	GlassNoRaytrace,
	CastShadowsOntoInvisible,

	WAVEFRONT_ILLUMMINATION_MAX_ENUMS
};
struct WavefrontMtl 
{
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
	float alpha;
	enum WavefrontIlluminationModel model;
};




struct WavefrontGroup
{
	uint32_t ixFirst;	// Index of the first element(AKA index) in the parent WavefrontObj's indices vector.
	uint32_t count;	// Index of the last element(AKA index) in the parent WavefrontObj's indices vector.
	char* name;
	char* mtlName;
};
inline void wfDestroyGroup(struct WavefrontGroup* g)
{
	if(g->mtlName) _aligned_free(g->mtlName);
	if (g->name) _aligned_free(g->name);
}



struct WavefrontObj 
{
	struct Vertex* vertices;	// stb_ds darray
	uint32_t* indices;
	struct WavefrontGroup* groups;
	
	
	char* name;
	char* mtllib;		// mtllib
};
inline void wfDestroyObj(struct WavefrontObj* o) 
{
	arrfree(o->vertices);
	arrfree(o->indices);
	uint32_t groupCount = arrlenu(o->groups);
	for (uint32_t i = 0; i < groupCount; i++)
		wfDestroyGroup(&o->groups[i]);
	if (o->name)
	{
		_aligned_free(o->name);
		o->name = NULL;
	}
	if (o->mtllib) 
	{
		_aligned_free(o->mtllib);
		o->mtllib = NULL;
	}
}



struct WavefrontMesh wavefront_read(_In_ FILE* file);

struct WavefrontObj wavefront_obj_read(_In_ FILE* file);

// str -> WavefrontMtl table
struct WavefrontMtllibKV
{
	_Null_terminated_ char* key;
	struct WavefrontMtl value;
};
struct WavefrontMtllib {
	struct WavefrontMtllibKV* materialMap;	//stb_ds c string map
	char const** keys;	// stb_ds darray of c strings
};
inline void wfDestroyMtllib(struct WavefrontMtllib* mtllib)
{
	uint32_t keyCount = arrlenu(mtllib->keys);
	if (mtllib->keys)
		for (uint32_t i = 0; i < keyCount; i++)
			_aligned_free(mtllib->keys[i]);
	
	arrfree(mtllib->keys);
	shfree(mtllib->materialMap);
}

/**
	@brief reads 
	@param   file 
	@returns stb_ds string->WavefrontMtl hashmap.
**/
struct WavefrontMtllib wavefront_mtl_read(_In_ FILE* file);