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
static _Success_(return != false) bool parseStringArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	char** strArr = (char**)userData;

	if (t != JSONtype_STRING)
		return false;

	strArr[ix] = copyStringHeap(v.string);

	return true;
}


#pragma region Accessor

static enum GLTFaccessor_type parseAccessorType(_In_z_ const char* type)
{
	return
		strcmp(type,"SCALAR")== 0 ? GLTFaccessor_type_SCALAR :

		strcmp(type, "VEC2") == 0 ? GLTFaccessor_type_VEC2 :
		strcmp(type, "VEC3") == 0 ? GLTFaccessor_type_VEC3 :
		strcmp(type, "VEC4") == 0 ? GLTFaccessor_type_VEC4 :

		strcmp(type, "MAT2") == 0 ? GLTFaccessor_type_MAT2 :
		strcmp(type, "MAT3") == 0 ? GLTFaccessor_type_MAT3 :
		strcmp(type, "MAT4") == 0 ? GLTFaccessor_type_MAT4 :

		GLTFaccessor_type_SCALAR;
}
static void parseAccessorMinMax(_In_ uint32_t count, _Out_writes_(min(count, 16)) union GLTFaccessor_minmax* minmax, _In_opt_ JSONarray a, GLenum ctype)
{
	if (!a) return;
	for (int i = 0; i < count; i++)
	{
		int32_t integral = jsonArrayGet(a, i).integer; //will give nonsensical answer if type is Float

		if (ctype == GL_FLOAT)
		{
			minmax[i].f32 = (float)jsonArrayGet(a, i).number;
		}
		else switch (ctype)
		{
		case GL_UNSIGNED_BYTE:
			minmax[i].u8 = (uint8_t)integral;
			break;
		case GL_BYTE:
			minmax[i].i8 = (int8_t)integral;
			break;
		case GL_UNSIGNED_SHORT:
			minmax[i].u16 = (uint16_t)integral;
			break;
		case GL_SHORT:
			minmax[i].i16 = (int16_t)integral;
			break;
		case GL_UNSIGNED_INT:
			minmax[i].u32 = (uint32_t)integral;
			break;
		case GL_INT:
			minmax[i].i32 = (uint32_t)integral;
			break;
		default:
			break;
		}

	}


}
static struct GLTFaccessor_sparse parseAccessorSparse(_In_opt_ JSONobject o, _In_ const struct GLTFbufferView* bufferViews)
{
	if (!o) return (struct GLTFaccessor_sparse) { .count = 0 }; //0 means its invalid
	uint32_t count = jsonObjectGet(o, "count").uint;
	JSONobject 
		indices = jsonObjectGet(o, "indices").object,
		 values = jsonObjectGet(o, "values").object;
	assert(indices && values);

	struct GLTFbufferView
		* indices_bw = bufferViews + jsonObjectGet(indices, "bufferView").uint,
		*  values_bw = bufferViews + jsonObjectGet( values, "bufferView").uint;
	uint32_t
		indices_offset = jsonObjectGetOrElse(indices, "byteOffset", JVU(uint = 0)).uint,
		 values_offset = jsonObjectGetOrElse( values, "byteOffset", JVU(uint = 0)).uint;
	
	return (struct GLTFaccessor_sparse) 
	{
		.count = count,
			.indices_bufferView = indices_bw,
			.values_bufferView = values_bw,
			
			.indices_byteOffset = indices_offset,
			.values_bufferView = values_offset,

			.indices_componentType = (GLenum)jsonObjectGet(indices, "componentType").uint,
	};
}

struct AccessorReadCtx
{
	struct GLTFaccessor* accessors;

	const struct GLTFbufferView* bufferViews;
};
static _Success_(return != false) bool parseAccessorArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct AccessorReadCtx* ctx = (struct AccessorReadCtx*)userData;
	JSONobject o = v.object;


	struct GLTFaccessor acc = {
		.bufferView = ctx->bufferViews + jsonObjectGetOrElse(o,"bufferView", JVU(uint = 0)).uint,
		.byteOffset = jsonObjectGetOrElse(o, "byteOffset", JVU(uint = 0)).uint,
		.componenType = jsonObjectGet(o, "componentType").uint,
		.normalized = jsonObjectGetOrElse(o, "normalized", JVU(boolean = false)).boolean,
		.count = jsonObjectGet(o, "count").uint,
		.type = parseAccessorType(jsonObjectGet(o, "type").string),
		.sparse = parseAccessorSparse(jsonObjectGetOrElse(o, "sparse", JVU(object = NULL)).object, ctx->bufferViews),
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", JVU(string = NULL) ).string)
	};
	uint32_t minmaxC = _gltfAccessorTypeComponentCount(acc.type);
	parseAccessorMinMax(minmaxC, acc.max, jsonObjectGetOrElse(o, "max", JVU(array = NULL)).array, acc.componenType);
	parseAccessorMinMax(minmaxC, acc.min, jsonObjectGetOrElse(o, "min", JVU(array = NULL)).array, acc.componenType);

	ctx->accessors[ix] = acc;


	return true;
}

static void destroyAccessor(struct GLTFaccessor* pAccessor)
{
	if (pAccessor->name)
		free(pAccessor->name);
}

#pragma endregion


#pragma region Animation

static enum GLTFanimation_sampler_interpolation animationSamplerInterpolationFromString(_In_z_ const char* i)
{
	return 
		strcmp(i, "LINEAR") == 0 ? GLTFanimation_sampler_interpolation_LINEAR :
		strcmp(i, "STEP")	== 0 ? GLTFanimation_sampler_interpolation_STEP :
		strcmp(i, "CUBICSPLINE") == 0 ? GLTFanimation_sampler_interpolation_CUBICSPLINE : 
		
		GLTFanimation_sampler_interpolation_max;
}
struct AnimationSamplerReadCtx
{
	struct GLTFanimation_sampler* samplers;

	const struct GLTFaccessor* accessors;
};
static _Success_(return != false) bool parseAnimationSamplerCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct AnimationSamplerReadCtx* ctx = (struct AnimationSamplerReadCtx*)userData;
	JSONobject o = v.object;

	struct GLTFanimation_sampler sampler = {
		. input = ctx->accessors + jsonObjectGet(o,  "input").uint,
		.output = ctx->accessors + jsonObjectGet(o, "output").uint,

		.interpolation = animationSamplerInterpolationFromString(jsonObjectGetOrElse(o, "interpolation", JVU(string = "LINEAR")).string),

	};
	ctx->samplers[ix] = sampler;


	return true;
}


static enum GLTFanimation_channel_target_path animationChannelTargetPathFromString(const char* path)
{
	return 
		strcmp(path, "translation") == 0 ? GLTFanimation_channel_target_path_TRANSLATION : 
		strcmp(path, "rotation")	== 0 ? GLTFanimation_channel_target_path_ROTATION :
		strcmp(path, "scale")		== 0 ? GLTFanimation_channel_target_path_SCALE : 
		strcmp(path, "weights")		== 0 ? GLTFanimation_channel_target_path_WEIGHTS :

		GLTFanimation_channel_target_path_max;
}
struct AnimationChannelReadCtx
{
	struct GLTFanimation_channel* channels;

	const struct GLTFanimation_sampler* samplers;
	const struct GLTFnode* nodes;
};
static _Success_(return != false) bool parseAnimationChannelArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct AnimationChannelReadCtx* ctx = (struct AnimationChannelReadCtx*)userData;
	JSONobject
		o = v.object,
		target = jsonObjectGet(o, "target").object;

	struct GLTFanimation_channel c = {
		.sampler = ctx->samplers + jsonObjectGet(o, "sampler").uint,
		
		.target_node = jsonObjectGetType(target, "node") == JSONtype_INTEGER ? ctx->nodes + jsonObjectGet(target, "node").uint : NULL,
		.target_path = animationChannelTargetPathFromString(jsonObjectGet(target, "path").string),
	};
	ctx->channels[ix] = c;

	return true;
}


struct AnimationReadCtx
{
	struct GLTFanimation* animations;

	const struct GLTFaccessor* accessors;
	const struct GLTFnode* nodes;
};
static _Success_(return != false) bool parseAnimationArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct AnimationReadCtx* ctx = (struct AnimatonReadCtx*)userData;
	JSONobject
		o = v.object;
	JSONarray
		channels = jsonObjectGet(o, "channels").array,
		samplers = jsonObjectGet(o, "samplers").array;

	struct GLTFanimation an = {
		.channels = NULL,
		.samplers = NULL,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", JVU(string = NULL)).string)
	};
	arrsetlen(an.channels, jsonArrayGetSize(channels));
	arrsetlen(an.samplers, jsonArrayGetSize(samplers));


	struct AnimationSamplerReadCtx samplerCtx = { 
		.samplers = an.samplers, 
		.accessors = ctx->accessors 
	};
	if (jsonArrayForeach(samplers, parseAnimationSamplerCallback, &samplerCtx) != 0)
	{
		//parsing failed
		arrfree(an.samplers);
		return false;
	}		

	struct AnimationChannelReadCtx channelCtx = {
		.channels = an.channels,
		.samplers = an.samplers,
		.nodes = ctx->nodes,
	};
	if (jsonArrayForeach(channels, parseAnimationChannelArrayCallback, &channelCtx) != 0)
	{
		//parsing failed!
		arrfree(an.channels);
		return false;
	}
		

	ctx->animations[ix] = an;
	return true;
}

static void destroyAnimation(struct GLTFanimation* pAnimation)
{
	arrfree(pAnimation->channels);
	arrfree(pAnimation->samplers);

	if (pAnimation->name)
		free(pAnimation->name);
}

#pragma endregion


#pragma region Buffer

struct BufferReadCtx {
	struct GLTFbuffer* buffers;

	bool isGLB;
	void* glb;
};
static _Success_(return != false) bool parseBufferArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct BufferReadCtx* ctx = (struct BufferReadCtx*)userData;
	JSONobject o = v.object;

	struct GLTFbuffer b = {
		.byteLength = jsonObjectGet(o, "byteSize").uint,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", JVU(string = NULL)).string)
	};
	if (ix == 0 && ctx->isGLB)
	{
		b.data = ctx->glb;
	}
	else
	{
		b.uri = copyStringHeap(jsonObjectGet(o, "uri", JVU(string = NULL)).string);
	}

	return true;
}

static void destroyBuffer(struct GLTFbuffer* b)
{
	if (b->name)
		free(b->name);
	
	if (b->uri)
		free(b->uri);
}

#pragma endregion


#pragma region BufferView

#pragma endregion


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

	if (accessors && bufferViews)
	{
		struct AccessorReadCtx ctx = { .accessors = gltf.accessors, .bufferViews = gltf.bufferViews };
		jsonArrayForeach(accessors, parseAccessorArrayCallback, &ctx);
	}
	if (animations && accessors && nodes)
	{
		struct AnimationReadCtx ctx = { .animations = gltf.animations, .accessors = gltf.accessors, .nodes = gltf.nodes };
		jsonArrayForeach(animations, parseAnimationArrayCallback, &ctx);
	}
	if (buffers)
	{
		struct BufferReadCtx ctx = { .buffers = gltf.buffers, .isGLB = isGLB, .glb = glbData };
		jsonArrayForeach(buffers, parseBufferArrayCallback, &ctx);
	}


	

	




}

extern void gltfClear(_In_ struct GLTFgltf* gltf)
{

}

extern void* gltfDataUriToBuffer(_In_z_ const char* data)
{
	GLTFname
		DATA_PFX = "data:",
		MIME_TYPE = "application",
		SUBTYPE_ALT1 = "octet-stream",
		SUBTYPE_ALT2 = "gltf-buffer",
		ENCODING = "base64"
		;

	// either
	//"data:application/octet-stream";
	// or
	//"data:application/gltf-buffer";

	char* it = data;

	assert(strncmp(it, DATA_PFX, strlen(DATA_PFX)) == 0);
	it += strlen(DATA_PFX);

	assert(strncmp(it, MIME_TYPE, strlen(MIME_TYPE)) == 0);
	it += strlen(MIME_TYPE);

	assert(*it == '/');
	it++;

	if (strncmp(it, SUBTYPE_ALT1, strlen(SUBTYPE_ALT1)) == 0)
	{
		it += strlen(SUBTYPE_ALT1);
	}
	else if (strncmp(it, SUBTYPE_ALT2, strlen(SUBTYPE_ALT2)) == 0)
	{
		it += strlen(SUBTYPE_ALT2);
	}
	else __debugbreak();

	assert(*it == ';');
	it++;

	assert(strncmp(it, ENCODING, strlen(ENCODING)) == 0);
	it += strlen(ENCODING);

	assert(*it == ',');
	it++;

	//TODO
	// see https://en.wikipedia.org/wiki/Base64

	return NULL;
}