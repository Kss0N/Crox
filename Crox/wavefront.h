#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <linmath.h>
#include "mesh.h"

#include <stb_image.h>
#include <stb_ds.h>

typedef float* stbi_bmp;

struct WavefrontMesh {
	struct Vertex* vertices;	// stb_ds array
	uint32_t* indices;			// stb_ds array

	const char* objName;		
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
struct WavefrontMtl {

	union {
		struct {
			vec3 ambient;
			vec3 diffuse;
			vec3 spectral;
		};
		struct {
			stbi_bmp* ambientMap;
			stbi_bmp* diffuseMap;
			stbi_bmp* spectralMap;
		};

	};

	union {
		vec3 specular;
		float alpha;	// in float[0,1] inclusive
		struct {
			stbi_bmp* specularAlphaMap; //this map contains specularity on R-channel and transparency on A-channel
			float saNan; // -value => specular (no bmp), +val => specular + alpha (no bmp), -NaN => Specular  bmp, +NaN => specular+Alpha bmp.
		};
	};
	uint32_t shininess;	// in int[0,1000] inclusive
	vec3 transFilter;
	float indexOfRefraction;
	enum WavefrontIlluminationModel model;
};

struct WavefrontData 
{
	struct Wavefront* meshes;	// stb_ds array

	const char* mtllibs;		// stb_ds array of mtllib paths used

};

// str -> WavefrontMtl 
struct WavefrontMtllibKV
{
	_Null_terminated_ char* key;
	struct WavefrontMtl value;
};

struct WavefrontMesh wavefront_read(_In_ FILE* file);


/**
	@brief reads 
	@param   file 
	@returns stb_ds string->WavefrontMtl hashmap.
**/
struct WavefrontMtllibKV* wavefront_mtl_read(_In_ FILE* file);