#include <glad/gl.h>
#include "gltf.h"
#include "json.h"
#include <stb_ds.h>

#define assert(expr) if(!(expr)) __debugbreak();

char* copyStringHeap(const char* str)
{
	if (!str) return NULL;

	char* dst = malloc((strlen(str) + 1) * sizeof * str);
	if (dst)
	{
		dst[strlen(str)] = '\0';
		strcpy(dst, str);
	}
	return dst;
}


struct StringArrayReadCtx
{
	uint32_t unused;
};
_Success_(return != false) bool parseStringArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	char** strArr = (char**)userData;

	if (t != JSONtype_STRING)
		return false;

	strArr[ix] = copyStringHeap(v.string);

	return true;
}


struct AccessorReadCtx
{
	struct GLTFaccessor* accessors;

	const struct GLTFbufferView* bufferViews;
};
_Success_(return != false) bool parseAccessorArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct AccessorReadCtx* ctx = (struct AccessorReadCtx*)userData;


	return true;
}




;

extern struct GLTFgltf gltfRead(_In_ FILE* f, _In_ bool isGLB)
{
	JSONobject object = NULL;
	void* glbData = NULL;
	struct GLTFgltf gltf = {
		.extensionsUsed		= NULL,
		.extensionsRequired = NULL,
		.accessors			= NULL,
		.buffers			= NULL,
		.bufferViews		= NULL,
		.cameras			= NULL,
		.images				= NULL,
		.materials			= NULL,
		.meshes				= NULL,
		.nodes				= NULL,
		.samplers			= NULL,
		.scenes				= NULL,
		.skins				= NULL,
		.textures			= NULL,

		.lights				= NULL,
		.variants			= NULL,
		.emitters			= NULL,
		.sources			= NULL,

		.asset = {
			.copyright	= NULL,
			.generator	= NULL,
			.versionMajor = 0,
			.versionMinor = 0,

			.minVersionMajor = 0,
			.minVersionMinor = 0,
	},
		.scene = NULL,

	};

	if (isGLB)
	{
		uint32_t magic;
		fread(&magic, sizeof magic, 1, f);
		if (magic != 0x46546C67) 
			__debugbreak();

		uint32_t version;
		fread(&version, sizeof version, 1, f);

		uint32_t length;
		fread(&length, sizeof length, 1, f);

		uint32_t jsonLength;
		fread(&jsonLength, sizeof jsonLength, 1, f);

		uint32_t jsonChunkType;
		fread(&jsonChunkType, sizeof jsonChunkType, 1, f);
		assert(jsonChunkType == 0x4E4F534A);

		char* jsonString = (char*)malloc((jsonLength + 1) * sizeof(char));
		if (!jsonString)
			return gltf;
		fread_s(jsonString, jsonLength, sizeof * jsonString, jsonLength, f);
		jsonString[jsonLength] = '\0';

		object = jsonParseValue(jsonString, UINT8_MAX, NULL).object;
		free(jsonString);

		if (!feof(f))
		{
			uint32_t dataLength;
			fread(&dataLength, sizeof dataLength, 1, f);
			
			uint32_t chunkType;
			fread(&chunkType, sizeof chunkType, 1, f);
			assert(chunkType == 0x004E4942);

			glbData = malloc(dataLength);
			if (!glbData)
				return gltf;
		}
	}
	else
	{
		fpos_t posBegin = ftell(f);
		fseek(f, 0, SEEK_END);
		size_t size = ftell(f) - posBegin;
		fseek(f, posBegin, SEEK_SET);

		char* jsonString = (char*)malloc( (size+1) * sizeof(char*));
		if (!jsonString) 
			return gltf;
		fread_s(jsonString, size * sizeof(char), sizeof(char), size, f);
		jsonString[size] = '\0';

		object = jsonParseValue(jsonString, UINT8_MAX, NULL).object;
		free(jsonString);
	}

	// Parse Asset

	assert(jsonObjectGetType(object, "asset") == JSONtype_OBJECT);
	JSONobject asset = jsonObjectGet(object, "asset").object;
	gltf.asset.copyright = copyStringHeap(jsonObjectGetOrElse(asset, "copyright", JVU(string = NULL)).string);
	gltf.asset.generator = copyStringHeap(jsonObjectGetOrElse(asset, "generator", JVU(string = NULL)).string);

	const char* itVersion = jsonObjectGet(asset, "version").string;
	gltf.asset.versionMajor = (uint32_t)strtoull(itVersion, itVersion, 10);
	assert(*itVersion == '.');
	gltf.asset.versionMinor = (uint32_t)strtoull(itVersion, NULL, 10);

	const char* itVersion = jsonObjectGetOrElse(asset, "minVersion", JVU(string = NULL)).string;
	if (itVersion)
	{
		gltf.asset.versionMajor = (uint32_t)strtoull(itVersion, itVersion, 10);
		assert(*itVersion == '.');
		gltf.asset.versionMinor = (uint32_t)strtoull(itVersion, NULL, 10);
	}

	JSONtype type;
#ifdef _DEBUG
	type = jsonObjectGetType(object, "extensionsUsed");		assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "extensionsRequired");	assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "accessors");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "animations");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "buffers");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "bufferView");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "cameras");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "images");				assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "materials");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "meshes");				assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "nodes");				assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "scenes");				assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "skins");				assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "textures");			assert(type == JSONtype_ARRAY || type == JSONtype_UNKNOWN);
	type = jsonObjectGetType(object, "extensions");			assert(type == JSONtype_OBJECT|| type == JSONtype_UNKNOWN);
#endif // _DEBUG

	;
	JSONarray
		extensionsUsed		= jsonObjectGetOrElse(object, "extensionsUsed",		JVU(array = NULL)).array,
		extensionsRequired	= jsonObjectGetOrElse(object, "extensionsRequired", JVU(array = NULL)).array,
		accessors			= jsonObjectGetOrElse(object, "accessors",			JVU(array = NULL)).array,
		animations			= jsonObjectGetOrElse(object, "animations",			JVU(array = NULL)).array,
		buffers				= jsonObjectGetOrElse(object, "buffers",			JVU(array = NULL)).array,
		bufferViews			= jsonObjectGetOrElse(object, "bufferViews",		JVU(array = NULL)).array,
		cameras				= jsonObjectGetOrElse(object, "cameras",			JVU(array = NULL)).array,
		images				= jsonObjectGetOrElse(object, "images",				JVU(array = NULL)).array,
		materials			= jsonObjectGetOrElse(object, "materials",			JVU(array = NULL)).array,
		meshes				= jsonObjectGetOrElse(object, "meshes",				JVU(array = NULL)).array,
		nodes				= jsonObjectGetOrElse(object, "nodes",				JVU(array = NULL)).array,
		samplers			= jsonObjectGetOrElse(object, "samplers",			JVU(array = NULL)).array,
		scenes				= jsonObjectGetOrElse(object, "scenes",				JVU(array = NULL)).array,
		skins				= jsonObjectGetOrElse(object, "skins",				JVU(array = NULL)).array,
		textures			= jsonObjectGetOrElse(object, "textures",			JVU(array = NULL)).array,
		lights  = NULL,
		variants= NULL,
		audio	= NULL,
		emitters= NULL,
		sources = NULL
		;
	JSONobject extensions = jsonObjectGetOrElse(object, "extensions", JVU(object = NULL)).object;
	if (extensions)
	{
#ifdef _DEBUG
		type = jsonObjectGetType(extensions, "KHR_lights_punctual");	assert(type == JSONtype_OBJECT || type == JSONtype_UNKNOWN);
		type = jsonObjectGetType(extensions, "KHR_materials_variants"); assert(type == JSONtype_OBJECT || type == JSONtype_UNKNOWN);
		type = jsonObjectGetType(extensions, "KHR_audio");				assert(type == JSONtype_OBJECT || type == JSONtype_UNKNOWN);
#endif // _DEBUG

		JSONobject khr_lights_punctual = jsonObjectGetOrElse(extensions, "KHR_lights_punctual", JVU(object = NULL)).object;
		if (khr_lights_punctual)
		{
			lights = jsonObjectGetOrElse(khr_lights_punctual, "lights", JVU(array = NULL)).array;
		}
		
		JSONobject khr_materials_variants = jsonObjectGetOrElse(extensions, "KHR_materials_variants", JVU(object = NULL)).object;
		if (khr_materials_variants)
		{
			variants = jsonObjectGetOrElse(khr_materials_variants, "variants", JVU(array = NULL)).array;
		}
		
		JSONobject khr_audio = jsonObjectGetOrElse(extensions, "KHR_audio", JVU(object = NULL)).object;
		if (khr_audio)
		{
			audio	= jsonObjectGetOrElse(khr_audio, "audio",	JVU(array = NULL)).array;
			emitters= jsonObjectGetOrElse(khr_audio, "emitters",JVU(array = NULL)).array;
			sources = jsonObjectGetOrElse(khr_audio, "sources", JVU(array = NULL)).array;
		}
	}

	if (extensionsUsed)		arrsetlen(gltf.extensionsUsed,		jsonArrayGetSize(extensionsUsed));
	if (extensionsRequired) arrsetlen(gltf.extensionsRequired,	jsonArrayGetSize(extensionsRequired));
	if (accessors)			arrsetlen(gltf.accessors,			jsonArrayGetSize(accessors));
	if (animations)			arrsetlen(gltf.animations,			jsonArrayGetSize(animations));
	if (buffers)			arrsetlen(gltf.buffers,				jsonArrayGetSize(buffers));
	if (bufferViews)		arrsetlen(gltf.bufferViews,			jsonArrayGetSize(bufferViews));
	if (cameras)			arrsetlen(gltf.cameras,				jsonArrayGetSize(cameras));
	if (images)				arrsetlen(gltf.images,				jsonArrayGetSize(images));
	if (materials)			arrsetlen(gltf.materials,			jsonArrayGetSize(materials));
	if (meshes)				arrsetlen(gltf.meshes,				jsonArrayGetSize(meshes));
	if (nodes)				arrsetlen(gltf.nodes,				jsonArrayGetSize(nodes));
	if (samplers)			arrsetlen(gltf.samplers,			jsonArrayGetSize(samplers));
	if (scenes)				arrsetlen(gltf.scenes,				jsonArrayGetSize(scenes));
	if (textures)			arrsetlen(gltf.textures,			jsonArrayGetSize(textures));
	if (lights)				arrsetlen(gltf.lights,				jsonArrayGetSize(lights));
	if (variants)			arrsetlen(gltf.variants,			jsonArrayGetSize(variants));
	if (audio)				arrsetlen(gltf.audio,				jsonArrayGetSize(audio));
	if (emitters)			arrsetlen(gltf.emitters,			jsonArrayGetSize(emitters));
	if (sources)			arrsetlen(gltf.sources,				jsonArrayGetSize(sources));

	if (extensionsUsed)
		jsonArrayForeach(extensionsUsed, parseStringArrayCallback, gltf.extensionsUsed);
	if (extensionsRequired)
		jsonArrayForeach(extensionsRequired, parseStringArrayCallback, gltf.extensionsRequired);




	

	




}

extern void gltfClear(_In_ struct GLTFgltf* gltf)
{

}