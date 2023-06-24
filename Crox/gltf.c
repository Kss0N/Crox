#include <glad/gl.h>
#include "gltf.h"
#include "json.h"
#include <stb_ds.h>
#include <float.h>
#define _USE_MATH_DEFINES
#include <math.h>

#define assert(expr) if(!(expr)) __debugbreak();

inline bool XOR(bool a, bool b)
{
	return (a || b) && !(a && b);
}

static char* copyStringHeap(const char* str)
{
	if (!str) return NULL;

	char* dst = malloc((strlen(str) + 1) * sizeof * str);
	if (dst)
	{
		dst[strlen(str)] = '\0';
		strcpy_s(dst, strlen(str)+1, str);
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

static enum GLTFmimeType mimeTypeFromString(const char* str)
{
	return
		strcmp(str, "image/png") == 0 ? GLTFmimeType_IMAGE_PNG :
		strcmp(str, "image/jpeg") == 0 ? GLTFmimeType_IMAGE_JPEG :
		strcmp(str, "audio/mpeg") == 0 ? GLTFmimeType_AUDIO_MPEG :

		GLTFmimeType_NONE;
}

static bool enumArrayContains(_In_ uint32_t count, _In_reads_(count) GLenum* enumV, GLenum _enum)
{
	for (int i = 0; i < count; i++)
	{
		if (enumV[i] == _enum)
			return true;
	}

	return false;
}

static const GLenum ALLOWED_COMP_TYPES[] = { GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, GL_UNSIGNED_INT, GL_INT, GL_FLOAT };

static const GLenum ALLOWED_BUFFER_VIEW_TARGET[] = {GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER};


static _Success_(return != false) bool parseVectorArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	float* vec = (float*)userData;

	vec[ix] = (t == JSONtype_NUMBER) ? (float)v.number : (float)v.integer;

	return true;
}

static bool parseVector(JSONarray a,  float* vector)
{
	return jsonArrayForeach(a, parseVectorArrayCallback, vector) != 0;
}

#define DEFAULT_TEXINFO_OFFSET {0,0}
#define DEFAULT_TEXINFO_ROTATION 0
#define DEFAULT_TEXINFO_SCALE {1,1}

#define DEFAULT_TEXCOORD 0

static struct GLTFtextureInfo parseTextureInfo(JSONobject o, const struct GLTFtexture* textures)
{
	JSONobject
		exts = o ? jsonObjectGetOrElse(o, "extensions", _JVU(NULL)).object : NULL,
		KHR_texture_transform = exts ? jsonObjectGetOrElse(o, "KHR_texture_transform", _JVU(NULL)).object : NULL;

	struct GLTFtextureInfo info =
	{
		.texture = o ? textures + jsonObjectGet(o, "index").uint : NULL,
		.texCoord= o ? jsonObjectGetOrElse(o, "texCoord", _JVU(DEFAULT_TEXCOORD)).uint : DEFAULT_TEXCOORD,

		.offset = DEFAULT_TEXINFO_OFFSET,
		
		.rotation = KHR_texture_transform ? jsonObjectGetOrElse(KHR_texture_transform, "rotation", JVU(number = DEFAULT_TEXINFO_ROTATION)).number : DEFAULT_TEXINFO_ROTATION,
		
		.scale = DEFAULT_TEXINFO_SCALE
	};
	if (KHR_texture_transform && jsonObjectGetType(KHR_texture_transform, "offset") == JSONtype_ARRAY)
		parseVector(jsonObjectGet(KHR_texture_transform, "offset").array, info.offset);

	if (KHR_texture_transform && jsonObjectGetType(KHR_texture_transform, "scale") == JSONtype_ARRAY)
		parseVector(jsonObjectGet(KHR_texture_transform, "scale").array, info.scale);

	return info;
}

struct NodeChildrenReadCtx
{
	struct GLTFnode** children;

	const struct GLTFnode* nodes;
};
static _Success_(return != false) bool parseNodeChildrenArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct NodeChildrenReadCtx* ctx = (struct NodeChildrenReadCtx*)userData;
	assert(ctx != NULL);
	if (t != JSONtype_INTEGER)
		return false;
	ctx->children[ix] = ctx->nodes + v.uint;
	return true;
}




#pragma region Accessor

static enum GLTFaccessor_type accessorTypeFromString(_In_z_ const char* type)
{
	return
		strcmp(type,"SCALAR")== 0 ? GLTFaccessor_type_SCALAR :

		strcmp(type, "VEC2") == 0 ? GLTFaccessor_type_VEC2 :
		strcmp(type, "VEC3") == 0 ? GLTFaccessor_type_VEC3 :
		strcmp(type, "VEC4") == 0 ? GLTFaccessor_type_VEC4 :

		strcmp(type, "MAT2") == 0 ? GLTFaccessor_type_MAT2 :
		strcmp(type, "MAT3") == 0 ? GLTFaccessor_type_MAT3 :
		strcmp(type, "MAT4") == 0 ? GLTFaccessor_type_MAT4 :

		GLTFaccessor_type_max;
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
		.bufferView = jsonObjectGetType(o, "bufferView") == JSONtype_INTEGER ? ctx->bufferViews + jsonObjectGet(o,"bufferView").uint : NULL,
		.byteOffset = jsonObjectGetOrElse(o, "byteOffset", JVU(uint = 0)).uint,
		.componenType = jsonObjectGet(o, "componentType").uint,
		.normalized = jsonObjectGetOrElse(o, "normalized", JVU(boolean = false)).boolean,
		.count = jsonObjectGet(o, "count").uint,
		.type = accessorTypeFromString(jsonObjectGet(o, "type").string),
		.sparse = parseAccessorSparse(jsonObjectGetOrElse(o, "sparse", JVU(object = NULL)).object, ctx->bufferViews),
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", JVU(string = NULL) ).string)
	};
	uint32_t minmaxC = _gltfAccessorTypeComponentCount(acc.type);
	parseAccessorMinMax(minmaxC, acc.max, jsonObjectGetOrElse(o, "max", JVU(array = NULL)).array, acc.componenType);
	parseAccessorMinMax(minmaxC, acc.min, jsonObjectGetOrElse(o, "min", JVU(array = NULL)).array, acc.componenType);

	
	assert(enumArrayContains(_countof(ALLOWED_COMP_TYPES), ALLOWED_COMP_TYPES, acc.componenType));

	assert(acc.count > 0);

	assert(acc.type != GLTFaccessor_type_max);

	if (acc.sparse.count > 0)
		assert(enumArrayContains(_countof(ALLOWED_COMP_TYPES), ALLOWED_COMP_TYPES, acc.sparse.indices_componentType));

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

#define DEFAULT_ANIMATION_SAMPLER_INTERPOLATION "LINEAR"

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

	assert(jsonObjectGetType(o,  "input") == JSONtype_INTEGER);
	assert(jsonObjectGetType(o, "output") == JSONtype_INTEGER);

	struct GLTFanimation_sampler sampler = {
		. input = ctx->accessors + jsonObjectGet(o,  "input").uint,
		.output = ctx->accessors + jsonObjectGet(o, "output").uint,

		.interpolation = animationSamplerInterpolationFromString(jsonObjectGetOrElse(o, "interpolation", JVU(string = DEFAULT_ANIMATION_SAMPLER_INTERPOLATION)).string),

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
	assert(target != NULL);
	assert(jsonObjectGetType(o, "sampler") == JSONtype_INTEGER);

	struct GLTFanimation_channel c = {
		.sampler = ctx->samplers + jsonObjectGet(o, "sampler").uint,
		
		.target_node = jsonObjectGetType(target, "node") == JSONtype_INTEGER ? ctx->nodes + jsonObjectGet(target, "node").uint : NULL,
		.target_path = animationChannelTargetPathFromString(jsonObjectGet(target, "path").string),
	};
	
	assert(c.target_path != GLTFanimation_channel_target_path_max);
	
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
	if (jsonArrayForeach(samplers, parseAnimationSamplerCallback, &samplerCtx) == 0)
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
	if (jsonArrayForeach(channels, parseAnimationChannelArrayCallback, &channelCtx) == 0)
	{
		//parsing failed!
		arrfree(an.channels);
		return false;
	}
	
	assert(arrlenu(an.channels) > 0);
	assert(arrlenu(an.samplers) > 0);

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


#pragma region Audio
//TODO
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
		.byteLength = jsonObjectGet(o, "byteLength").uint,
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

	assert(b.byteLength > 0);

	ctx->buffers[ix] = b;
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

struct BufferViewReadCtx
{
	struct GLTFbufferView* bufferViews;

	const struct GLTFbuffer* buffers;
};
static _Success_(return != false) bool parseBufferViewArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct BufferViewReadCtx* ctx = (struct BufferViewReadCtx*)userData;
	JSONobject o = v.object;

	assert(jsonObjectGetType(o, "buffer") == JSONtype_INTEGER);

	struct GLTFbufferView bw = {
		.buffer = ctx->buffers + jsonObjectGet(o, "buffer").uint,
		.byteOffset =  jsonObjectGetOrElse(o, "byteOffset", _JVU(0)).uint,
		.byteLength =  jsonObjectGet(o, "byteLength").uint,
		.byteStride =  jsonObjectGetOrElse(o, "byteStride", JVU(uint = 0)).uint,
		.target =  (GLenum)jsonObjectGetOrElse(o, "target", JVU(uint = 0)).uint,
		
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", JVU(string = 0)).string),		
	};
	assert(bw.byteLength > 0);
	
	if (bw.byteStride != 0)
		assert(4 <= bw.byteStride && bw.byteStride <= 252);
	
	if (bw.target != 0)
		assert(enumArrayContains(_countof(ALLOWED_BUFFER_VIEW_TARGET), ALLOWED_BUFFER_VIEW_TARGET, bw.target));

	ctx->bufferViews[ix] = bw;
	return true;
}
static void destroyBufferView(struct GLTFbufferView* bw)
{
	if (bw->name)
		free(bw->name);
}

#pragma endregion


#pragma region Camera

struct CameraReadCtx
{
	struct GLTFcamera* cameras;
};
static _Success_(return != false) bool parseCameraArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct CameraReadCtx* ctx = (struct CameraReadCtx*)userData;

	JSONobject o = v.object,
		perspective = jsonObjectGetOrElse(o, "perspective", _JVU(NULL)).object,
		orthographic= jsonObjectGetOrElse(o, "orthographic",_JVU(NULL)).object;
	assert(XOR(perspective != NULL, orthographic != NULL));

	struct GLTFcamera c = {
		.isPerspective = perspective != NULL,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string)
	};

	if (c.isPerspective)
	{
		c.perspective = (struct GLTFcamera_perspective){
			.aspectRatio= jsonObjectGetOrElse(perspective, "aspectRatio",	JVU(number = NAN))	.number,
			.yfov		= jsonObjectGet		 (perspective, "yfov")								.number,
			.zfar		= jsonObjectGet		 (perspective, "zfar")								.number,
			.znear		= jsonObjectGetOrElse(perspective, "znear",			JVU(number = NAN))	.number,
		};
	}
	else
		c.orthographic = (struct GLTFcamera_orthographic){
			.xmag = jsonObjectGet(orthographic, "xmag").number,
			.ymag = jsonObjectGet(orthographic, "ymag").number,
			.zfar = jsonObjectGet(orthographic, "zfar").number,
			.znear= jsonObjectGet(orthographic,"znear").number
		};

	ctx->cameras[ix] = c;
	return true;
}
static void destroyCamera(struct GLTFcamera* c)
{
	if (c->name)
		free(c->name);
}

#pragma endregion


#pragma region Image

struct ImageReadCtx
{
	struct GLTFimage* images;

	const struct GLTFbufferView* bufferViews;
};
static _Success_(return != false) bool parseImageArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct ImageReadCtx* ctx = (struct ImageReadCtx*)userData;
	JSONobject o = v.object;

	struct GLTFimage img = {
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),
	};

	if(jsonObjectGetType(o, "uri") == JSONtype_STRING)
	{ 
		img.uri = copyStringHeap(jsonObjectGet(o, "uri").string);
	}
	else if(jsonObjectGetType(o, "bufferView") == JSONtype_INTEGER)
	{
		img.bufferView = ctx->bufferViews + jsonObjectGet(o, "bufferView").uint;
		img.mimeType = mimeTypeFromString(jsonObjectGet(o, "mimeType").string);
	}

	ctx->images[ix] = img;
	return true;
}
static void destroyImage(struct GLTFimage* img)
{
	if (img->name) 
		free(img->name);
	if (img->mimeType == GLTFmimeType_NONE)
		free(img->uri);
}

#pragma endregion


#pragma region Material

#define DEFAULT_BASE_COLOR {1.f, 1.f, 1.f, 1.f}
#define DEFAULT_METALLIC_FACTOR 1.f
#define DEFAULT_ROUGHNESS_FACTOR 1.f
#define DEFAULT_NORMAL_SCALE 1.f
#define DEFAULT_OCCLUSION_STRENGTH 1.f
#define DEFAULT_EMISSIVE_FACTOR {1.f, 1.f, 1.f}
#define DEFAULT_ALPHA_CUTOFF .5f

#define DEFAULT_ANISOTROPY_STRENGTH 0
#define DEFAULT_ANISOTROPY_ROTATION 0

#define DEFAULT_DIFFUSE_TRANSMISSION_COLOR_FACTOR {1, 1, 1}

#define DEFAULT_EMISSIVE_STRENGTH 1

#define DEFAULT_IOR 1.5

#define DEFAULT_IRIDESCENCE_IOR 1.3
#define DEFAULT_IRIDESCENCE_THICKNESS_MIN 100
#define DEFAULT_IRIDESCENCE_THICKNESS_MAX 400

#define DEFAULT_SHEEN_COLOR_FACTOR {0,0,0}

#define DEFAULT_SPECULAR_FACTOR 1.f
#define DEFAULT_SPECULAR_COLOR_FACTOR {1.f, 1.f, 1.f}

#define DEFAULT_SCATTER_DISTANCE INFINITY
#define DEFAULT_SCATTER_COLOR {0,0,0}

#define DEFAULT_ATTENUATION_DISTANCE INFINITY
#define DEFAULT_ATTENUATION_COLOR {1,1,1}


static enum GLTFmaterial_alphaMode alphaModeFromSting(const char* str)
{
	return
		strcmp(str, "OPAQUE")	== 0 ? GLTFmaterial_alphaMode_OPAQUE : 
		strcmp(str, "MASK")		== 0 ? GLTFmaterial_alphaMode_MASK : 
		strcmp(str, "BLEND")	== 0 ? GLTFmaterial_alphaMode_BLEND :
		GLTFmaterial_alphaMode_max;
}

struct MaterialReadCtx
{
	struct GLTFmaterial* materials;

	struct GLTFtexture* textures;
};
static _Success_(return != false) bool parseMaterialArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct MaterialReadCtx* ctx = (struct MaterialReadCtx*)userData;
	const JSONobject o = v.object,
		pbr = jsonObjectGetOrElse(o, "pbrMetallicRoughness",_JVU(NULL)).object,
		 nt = jsonObjectGetOrElse(o, "normalTexture",		_JVU(NULL)).object,
		 ot = jsonObjectGetOrElse(o, "occlusionTexture",	_JVU(NULL)).object,
		 et = jsonObjectGetOrElse(o, "emissiveTexture",		_JVU(NULL)).object,
		bct = pbr ? jsonObjectGetOrElse(pbr, "baseColorTexture", _JVU(NULL)).object : NULL,
		mrt = pbr ? jsonObjectGetOrElse(pbr, "metallicRoughnessTexture", _JVU(NULL)).object : NULL,

		exts = jsonObjectGetOrElse(o, "extensions",	_JVU(NULL)).object,
		KHR_materials_anisotropy			= exts ? jsonObjectGetOrElse(exts, "KHR_materials_anisotropy",			_JVU(NULL)).object : NULL,
		KHR_materials_clearcoat				= exts ? jsonObjectGetOrElse(exts, "KHR_materials_clearcoat",			_JVU(NULL)).object : NULL,
		KHR_materials_diffuse_transmission	= exts ? jsonObjectGetOrElse(exts, "KHR_materials_diffuse_transmission",_JVU(NULL)).object : NULL,
		KHR_materials_emissive_strength		= exts ? jsonObjectGetOrElse(exts, "KHR_materials_emissive_strength",	_JVU(NULL)).object : NULL,
		KHR_materials_ior					= exts ? jsonObjectGetOrElse(exts, "KHR_materials_ior",					_JVU(NULL)).object : NULL,
		KHR_materials_iridescence			= exts ? jsonObjectGetOrElse(exts, "KHR_materials_iridescence",			_JVU(NULL)).object : NULL,
		KHR_materials_sheen					= exts ? jsonObjectGetOrElse(exts, "KHR_materials_sheen",				_JVU(NULL)).object : NULL,
		KHR_materials_specular				= exts ? jsonObjectGetOrElse(exts, "KHR_materials_specular",			_JVU(NULL)).object : NULL,
		KHR_materials_sss					= exts ? jsonObjectGetOrElse(exts, "KHR_materials_sss",					_JVU(NULL)).object : NULL,
		KHR_materials_transmission			= exts ? jsonObjectGetOrElse(exts, "KHR_materials_transmission",		_JVU(NULL)).object : NULL,
		KHR_materials_unlit					= exts ? jsonObjectGetOrElse(exts, "KHR_materials_unlit",				_JVU(NULL)).object : NULL,
		KHR_materials_volume				= exts ? jsonObjectGetOrElse(exts, "KHR_materials_volume",				_JVU(NULL)).object : NULL,
		//some more textures
		 at = KHR_materials_anisotropy ? jsonObjectGetOrElse(KHR_materials_anisotropy, "anisotropyTexture",			_JVU(NULL)).object : NULL,
		 ct = KHR_materials_clearcoat ? jsonObjectGetOrElse(KHR_materials_clearcoat, "clearcoatTexture",			_JVU(NULL)).object : NULL,
		crt = KHR_materials_clearcoat ? jsonObjectGetOrElse(KHR_materials_clearcoat, "clearcoatRoughnessTexture",	_JVU(NULL)).object : NULL,
		cnt = KHR_materials_clearcoat ? jsonObjectGetOrElse(KHR_materials_clearcoat, "clearcoatNormalTexture",		_JVU(NULL)).object : NULL,
		dtt = KHR_materials_diffuse_transmission ? jsonObjectGetOrElse(KHR_materials_diffuse_transmission, "diffuseTransmissionTexture", _JVU(NULL)).object : NULL,
		dtct= KHR_materials_diffuse_transmission ? jsonObjectGetOrElse(KHR_materials_diffuse_transmission, "diffuseTransmissionColorTexture", _JVU(NULL)).object : NULL,
		 it = KHR_materials_iridescence ? jsonObjectGetOrElse(KHR_materials_iridescence, "iridescenceTexture", _JVU(NULL)).object : NULL,
		itt = KHR_materials_iridescence ? jsonObjectGetOrElse(KHR_materials_iridescence, "iridescenceThicknessTexture", _JVU(NULL)).object : NULL,
		sct = KHR_materials_sheen ? jsonObjectGetOrElse(KHR_materials_sheen, "sheenColorTexture", _JVU(NULL)).object : NULL,
		srt = KHR_materials_sheen ? jsonObjectGetOrElse(KHR_materials_sheen, "sheenRoughnessTexture", _JVU(NULL)).object : NULL,
		spt = KHR_materials_specular ? jsonObjectGetOrElse(KHR_materials_specular, "specularTexture", _JVU(NULL)).object : NULL,
		spct= KHR_materials_specular ? jsonObjectGetOrElse(KHR_materials_specular, "specularColorTexture", _JVU(NULL)).object : NULL,
		tmt = KHR_materials_transmission ? jsonObjectGetOrElse(KHR_materials_transmission, "transmissionTexture", _JVU(NULL)).object : NULL,
		 tt = KHR_materials_transmission ? jsonObjectGetOrElse(KHR_materials_volume, "thicknessTexture", _JVU(NULL)).object : NULL
		;
	


	const struct GLTFtextureInfo DEFAULT_TEXTURE_INFO = parseTextureInfo(NULL, NULL);

	struct GLTFmaterial m = {
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),

		.baseColorFactor = DEFAULT_BASE_COLOR,
		.metallicFactor = pbr ? jsonObjectGetOrElse(pbr, "metallicFactor", JVU(number = DEFAULT_METALLIC_FACTOR)).number : 0,
		.roughnessFactor = pbr ? jsonObjectGetOrElse(pbr,"roughnessFactor", JVU(number = DEFAULT_ROUGHNESS_FACTOR)).number : 0,
		.normalScale = nt ? jsonObjectGetOrElse(nt, "scale", JVU(number = DEFAULT_NORMAL_SCALE)).number : 0,
		.occlusionStrength = ot ? jsonObjectGetOrElse(ot, "strength", JVU(number = DEFAULT_OCCLUSION_STRENGTH)).number : 0,
		.emissiveFactor = DEFAULT_EMISSIVE_FACTOR,
		.alphaCutoff = jsonObjectGetOrElse(o, "alphaCutoff", JVU(number = DEFAULT_ALPHA_CUTOFF)).number,

		.anisotropyStrength = KHR_materials_anisotropy ? jsonObjectGetOrElse(KHR_materials_anisotropy, "anisotropyStrength", JVU(number = DEFAULT_ANISOTROPY_STRENGTH)).number : DEFAULT_ANISOTROPY_STRENGTH,
		.anisotropyRotation = KHR_materials_anisotropy ? jsonObjectGetOrElse(KHR_materials_anisotropy, "anisotropyRotation", JVU(number = DEFAULT_ANISOTROPY_ROTATION)).number : DEFAULT_ANISOTROPY_ROTATION,

		.clearcoatFactor = KHR_materials_clearcoat ? jsonObjectGetOrElse(KHR_materials_clearcoat, "clearcoatFactor", _JVU(0)).number : 0,
		.clearcoatRoughnessFactor = KHR_materials_clearcoat ? jsonObjectGetOrElse(KHR_materials_clearcoat, "clearcoatRoughnessFactor", _JVU(0)).number : 0,
		.clearcoatNormalScale = cnt ? jsonObjectGetOrElse(cnt, "scale", JVU(number = DEFAULT_NORMAL_SCALE)).number : 0,

		.diffuseTransmissionFactor = KHR_materials_diffuse_transmission ? jsonObjectGetOrElse(KHR_materials_diffuse_transmission, "diffuseTransmissionFactor", _JVU(0)).number : 0,
		.diffuseTransmissionColorFactor = DEFAULT_DIFFUSE_TRANSMISSION_COLOR_FACTOR,

		.emissiveStrength = KHR_materials_emissive_strength ? jsonObjectGetOrElse(KHR_materials_emissive_strength, "emissiveStrength", JVU(number = DEFAULT_EMISSIVE_STRENGTH)).number : DEFAULT_EMISSIVE_STRENGTH,

		.ior = KHR_materials_ior ? jsonObjectGetOrElse(KHR_materials_ior, "ior", JVU(number = DEFAULT_IOR)).number : DEFAULT_IOR,

		.iridescenceFactor = KHR_materials_iridescence ? jsonObjectGetOrElse(KHR_materials_iridescence, "iridescenceFactor", _JVU(0)).number : 0,
		.iridescenceIor = KHR_materials_iridescence ? jsonObjectGetOrElse(KHR_materials_iridescence, "iridescenceIor", JVU(number = DEFAULT_IRIDESCENCE_IOR)).number : DEFAULT_IRIDESCENCE_IOR,
		.iridescenceThicknessMinimum = KHR_materials_iridescence ? jsonObjectGetOrElse(KHR_materials_iridescence, "iridescenceThicknessMinimum", JVU(number = DEFAULT_IRIDESCENCE_THICKNESS_MIN)).number : 0,
		.iridescenceThicknessMaximum = KHR_materials_iridescence ? jsonObjectGetOrElse(KHR_materials_iridescence, "iridescenceThicknessMaximum", JVU(number = DEFAULT_IRIDESCENCE_THICKNESS_MAX)).number : 0,

		.sheenColorFactor = DEFAULT_SHEEN_COLOR_FACTOR,
		.sheenRoughnessFactor = KHR_materials_sheen ? jsonObjectGetOrElse(KHR_materials_sheen, "sheenRoughnessFactor", _JVU(0)).number : 0,

		.specularFactor = KHR_materials_specular ? jsonObjectGetOrElse(KHR_materials_specular, "specularFactor", JVU(number = DEFAULT_SPECULAR_FACTOR)).number : DEFAULT_SPECULAR_FACTOR,
		.specularColorFactor = DEFAULT_SPECULAR_COLOR_FACTOR,

		.scatterDistance = KHR_materials_sss ? jsonObjectGetOrElse(KHR_materials_sss, "scatterDistance", JVU(number = DEFAULT_SCATTER_DISTANCE)).number : DEFAULT_SCATTER_DISTANCE,
		.scatterColor = DEFAULT_SCATTER_COLOR,

		.transmissionFactor = KHR_materials_transmission ? jsonObjectGetOrElse(KHR_materials_transmission, "transmissionFactor", _JVU(0)).number : 0,

		.thicknessFactor = KHR_materials_volume ? jsonObjectGetOrElse(KHR_materials_volume, "thicknessFactor", _JVU(0)).number : 0,
		.attenuationDistance = KHR_materials_volume ? jsonObjectGetOrElse(KHR_materials_volume, "attenuationDistance", JVU(number = DEFAULT_ATTENUATION_DISTANCE)).number : DEFAULT_ATTENUATION_DISTANCE,
		.attenuationColor = DEFAULT_ATTENUATION_COLOR,


		.alphaMode			= alphaModeFromSting(jsonObjectGetOrElse(o, "alphaMode", _JVU("OPAQUE")).string),
		.doubleSided = jsonObjectGetOrElse(o, "doubleSided", _JVU(false)).boolean,
		.unlit = KHR_materials_unlit != NULL,

		.baseColorTexture		=bct ? parseTextureInfo(bct, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.metallicRoughnessFactor=mrt ? parseTextureInfo(mrt, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.normalTexture			= nt ? parseTextureInfo( nt, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.occlusionTexture		= ot ? parseTextureInfo( ot, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.emissiveTexture		= et ? parseTextureInfo( et, ctx->textures) : DEFAULT_TEXTURE_INFO,
		
		.anisotropyTexture		= at ? parseTextureInfo( at, ctx->textures) : DEFAULT_TEXTURE_INFO,
		
		.clearcoatTexture		= ct ? parseTextureInfo( ct, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.clearcoatRoughnessTexture=crt?parseTextureInfo(crt, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.clearcoatNormalTexture	=cnt ? parseTextureInfo(cnt, ctx->textures) : DEFAULT_TEXTURE_INFO,
		
		.diffuseTransmissionTexture=dtt?parseTextureInfo(dtt,ctx->textures) : DEFAULT_TEXTURE_INFO,
		.diffuseTransmissionColorTexture=dtct?parseTextureInfo(dtct,ctx->textures):DEFAULT_TEXTURE_INFO,

		.iridescenceTexture		= it ? parseTextureInfo(it, ctx->textures) : DEFAULT_TEXTURE_INFO,
		.iridescenceThicknessTexture=itt?parseTextureInfo(itt,ctx->textures):DEFAULT_TEXTURE_INFO,

		.sheenColorTexture		= sct? parseTextureInfo(sct,ctx->textures) : DEFAULT_TEXTURE_INFO,
		.sheenRoughnessTexture	= srt? parseTextureInfo(srt,ctx->textures) : DEFAULT_TEXTURE_INFO,

		.specularTexture	  =   spt? parseTextureInfo(spt, ctx->textures):DEFAULT_TEXTURE_INFO,
		.specularColorTexture =  spct? parseTextureInfo(spct,ctx->textures):DEFAULT_TEXTURE_INFO,

		.transmissionTexture  =	 tmt ? parseTextureInfo(tmt, ctx->textures) : DEFAULT_TEXTURE_INFO,

		.thicknessTexture	   = tt	 ? parseTextureInfo(tt, ctx->textures) : DEFAULT_TEXTURE_INFO,
	};
	if (jsonObjectGetType(pbr, "baseColorFactor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(pbr, "baseColorFactor").array, m.baseColorFactor);
	}
	if (jsonObjectGetType(o, "emissiveFactor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(o, "emissiveFactor").array, m.emissiveFactor);
	}
	if (jsonObjectGetType(KHR_materials_diffuse_transmission, "diffuseTransmissionColorFactor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(KHR_materials_diffuse_transmission, "diffuseTransmissionColorFactor").array, m.diffuseTransmissionColorFactor);
	}
	if (jsonObjectGetType(KHR_materials_sheen, "sheenColorFactor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(KHR_materials_sheen, "sheenColorFactor").array, m.sheenColorFactor);
	}
	if (jsonObjectGetType(KHR_materials_specular, "specularColorFactor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(KHR_materials_specular, "specularColorFactor").array, m.specularColorFactor);
	}
	if (jsonObjectGetType(KHR_materials_sss, "scatterColor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(KHR_materials_sss, "scatterColor").array, m.scatterColor);
	}
	if (jsonObjectGetType(KHR_materials_volume, "attenuationColor") == JSONtype_ARRAY)
	{
		parseVector(jsonObjectGet(KHR_materials_volume, "attenuationColor").array, m.attenuationColor);
	}


	ctx->materials[ix] = m;
	return true;
}
void destroyMaterial(struct GLTFmaterial* mtl)
{
	if (mtl->name)
		free(mtl->name);
}
#pragma endregion


#pragma region Mesh

struct MeshPrimitiveAttributeReadCtx
{
	struct GLTFmesh_primitive_attributesKV* attributes;
	
	const struct GLTFaccessor* accessors;
};
static _Success_(return != false) bool parseMeshPrimitiveAttributeTableCallback(_In_ JSONobject o, _In_z_ const char* key, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct MeshPrimitiveAttributeReadCtx* ctx = (struct MeshPrimitiveAttributeReadCtx*)userData;
	assert(ctx);

	if (t != JSONtype_INTEGER) return false;

	shput(ctx->attributes, key, ctx->accessors + v.uint);

	return true;
}


struct MeshPrimitiveTargetReadCtx
{
	struct GLTFmesh_primitive_targetKV* target;

	const struct GLTFaccessor* accessors;
};
static _Success_(return != false) bool parseMeshPrimitiveMorphTargetCallback(_In_ JSONobject o, _In_z_ const char* key, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct MeshPrimitiveTargetReadCtx* ctx = (struct MeshPrimitiveTargetReadCtx*)userData;
	assert(ctx);
	if (t != JSONtype_INTEGER) return false;

	shput(ctx->target, key, ctx->accessors + v.uint);
	return true;
}


struct MeshPrimitiveTargetArrayReadCtx
{
	struct GLTFmesh_primitive_targetKV** targets;

	const struct GLTFaccessor* accessors;
};
static _Success_(return != false) bool parseMeshPrimitiveMorphTargetArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct MeshPrimitiveTargetArrayReadCtx* ctx = (struct MeshPrimitiveTargetArrayReadCtx*)userData;
	assert(ctx);
	JSONobject o = v.object;

	struct GLTFmesh_primitive_targetKV* target = NULL;
	sh_new_strdup(target);

	struct MeshPrimitiveTargetReadCtx tCtx = { .target = target, .accessors = ctx->accessors };
	bool success = jsonObjectForeach(o, parseMeshPrimitiveMorphTargetCallback, &tCtx);
	if (success)
		ctx->targets[ix] = target;
	return success;
}


struct MeshPrimitiveReadCtx
{
	struct GLTFmesh_primitive* primitives;

	const struct GLTFaccessor	* accessors;
	const struct GLTFbufferView	* bufferViews;
	const struct GLTFmaterial	* materials;
};
static _Success_(return != false) bool parseMeshPrimitveArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct MeshPrimitiveReadCtx* ctx = (struct MeshPrimitiveReadCtx*)userData;
	assert(ctx);
	JSONobject o = v.object,
		exts = jsonObjectGetOrElse(o, "extensions", _JVU(NULL)).object,
		KHR_draco_mesh_compression = exts ? jsonObjectGetOrElse(exts, "KHR_draco_mesh_compression", _JVU(NULL)).object : NULL;
		;

	struct GLTFmesh_primitive p =
	{
		.attributes = NULL,
		.indices = jsonObjectGetType(o, "indices") == JSONtype_INTEGER ? ctx->accessors + jsonObjectGet(o,   "indices").uint : NULL,
		.material= jsonObjectGetType(o, "material")== JSONtype_INTEGER ? ctx->materials + jsonObjectGet(o, "materials").uint : NULL,
		.targets = NULL,
		.mode = (GLenum)jsonObjectGetOrElse(o, "mode", _JVU(GL_TRIANGLES)).uint, 
		.compressionData = {
			.bufferView = KHR_draco_mesh_compression && jsonObjectGetType(KHR_draco_mesh_compression, "bufferView") == JSONtype_INTEGER ? ctx->bufferViews + jsonObjectGet(KHR_draco_mesh_compression, "bufferView").uint : NULL,
			.attributes = NULL,

		},
	};

	if (jsonObjectGetType(o, "attributes") == JSONtype_OBJECT)
	{
		sh_new_strdup(p.attributes);
		struct MeshPrimitiveAttributeReadCtx attribCtx = { .attributes = p.attributes, .accessors = ctx->accessors };
		bool success = jsonObjectForeach(jsonObjectGet(o, "attributes").object, parseMeshPrimitiveAttributeTableCallback, &attribCtx);

		if (!success) 
			return false;
	}

	JSONarray targets = jsonObjectGetOrElse(o, "targets", _JVU(NULL)).array;
	if (targets)
	{
		arrsetlen(p.targets, jsonArrayGetSize(targets));
		struct MeshPrimitiveTargetArrayReadCtx targetCtx = { .targets = p.targets, .accessors = ctx->accessors };
		bool success = jsonArrayForeach(targets, parseMeshPrimitiveMorphTargetArrayCallback, &targetCtx);
		
		if (!success)
			return false;
	}

	if (KHR_draco_mesh_compression && jsonObjectGetType(KHR_draco_mesh_compression, "attributes") == JSONtype_OBJECT)
	{
		sh_new_strdup(p.compressionData.attributes);
		JSONobject attribs = jsonObjectGet(KHR_draco_mesh_compression, "attributes").object;
		struct MeshPrimitiveAttributeReadCtx attribCtx = { .attributes = p.compressionData.attributes, .accessors = ctx->accessors };
		
		bool success = jsonObjectForeach(attribs, parseMeshPrimitiveAttributeTableCallback, &attribCtx);

		if (!success)
			return false;
	}

	return true;
}


struct MeshReadCtx
{
	struct GLTFmesh* meshes;

	
	const struct GLTFaccessor* accessors;
	const struct GLTFbufferView* bufferViews;
	const struct GLTFmaterial* materials;
};
static _Success_(return != false) bool parseMeshArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct MeshReadCtx* ctx = (struct MeshReadCtx*)userData;
	assert(ctx);

	JSONobject o = v.object;

	struct GLTFmesh m = {
		.primitives = NULL,
		.weights = NULL,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),
	};
	JSONarray
		primitives = jsonObjectGet(o, "primitives").array,
		weights = jsonObjectGetOrElse(o, "weights", _JVU(NULL)).array;
	
	if (weights)
	{
		arrsetlen(m.weights, jsonArrayGetSize(weights));
		bool success = parseVector(weights, m.weights); 
		if (!success)
			return false;
	}

	arrsetlen(m.primitives, jsonArrayGetSize(primitives));
	struct MeshPrimitiveReadCtx primitiveCtx = {
		.primitives = m.primitives, 
		.accessors = ctx->accessors, 
		.bufferViews = ctx->bufferViews, 
		.materials = ctx->materials };
	bool success = jsonArrayForeach(primitives, parseMeshPrimitveArrayCallback, &primitiveCtx) != 0;
	ctx->meshes[ix] = m;
	return success;
}

void destroyMesh(struct GLTFmesh* mesh)
{
	if (mesh->name)
		free(mesh->name);

	arrfree(mesh->weights);

	const size_t primitiveC = arrlenu(mesh->primitives);
	for (int i = 0; i < primitiveC; i++)
	{
		struct GLTFmesh_primitive p = mesh->primitives[i];
		shfree(p.attributes);

		const size_t targetC = arrlenu(p.targets);
		for (int j = 0; j < targetC; i++)
		{
			shfree(p.targets[j]);
		}
		arrfree(p.targets);

		shfree(p.compressionData.attributes);
	}

}

#pragma endregion


#pragma region Node

#define UNIT_MATRIX {1,0,0,0,	0,1,0,0,	0,0,1,0,	0,0,0,1}
#define UNIT_QUAT {0,0,0,1}
#define DEFAULT_SCALE {1,1,1}
#define ORGIN {0,0,0}

struct NodeReadCtx
{
	struct GLTFnode* nodes;

	const struct GLTFcamera		* cameras;
	const struct GLTFlightKHR	* lights;
	const struct GLTFmesh		* meshes;
	const struct GLTFskin		* skins;
};
static _Success_(return != false) bool parseNodeArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct NodeReadCtx* ctx = (struct NodeReadCtx*)userData;
	assert(ctx);
	JSONobject o = v.object;

	struct GLTFnode n = {
		.camera = jsonObjectGetType(o, "camera") == JSONtype_INTEGER ? ctx->cameras + jsonObjectGet(o, "camera").uint : NULL,
		.children = NULL,
		.skin = jsonObjectGetType(o, "skin") == JSONtype_INTEGER ? ctx-> skins + jsonObjectGet(o, "skin").uint : NULL,
		.mesh = jsonObjectGetType(o, "mesh") == JSONtype_INTEGER ? ctx->meshes + jsonObjectGet(o, "mesh").uint : NULL,
		.matrix = UNIT_MATRIX,
		.rotation = UNIT_QUAT,
		.scale = DEFAULT_SCALE,
		.translation = ORGIN,
		.weights = NULL,

		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),

		.light = jsonObjectGetType(o, "light") == JSONtype_INTEGER ? ctx->lights + jsonObjectGet(o, "light").uint : NULL,
	};

	JSONarray
		children	= jsonObjectGetOrElse(o, "children",	_JVU(NULL)).array,
		matrix		= jsonObjectGetOrElse(o, "matrix",		_JVU(NULL)).array,
		rotation	= jsonObjectGetOrElse(o, "rotation",	_JVU(NULL)).array,
		scale		= jsonObjectGetOrElse(o, "scale",		_JVU(NULL)).array,
		translation	= jsonObjectGetOrElse(o, "translation",	_JVU(NULL)).array,
		weights		= jsonObjectGetOrElse(o, "weights",		_JVU(NULL)).array;
	if (children)
	{
		arrsetlen(n.children, jsonArrayGetSize(children));

		struct NodeChildrenReadCtx childCtx = {
			.children = n.children,
			.nodes = ctx->nodes
		};
		bool success = jsonArrayForeach(children, parseNodeChildrenArrayCallback, &childCtx);
		if (!success)
			return success;
	}
	if (matrix)
	{
		bool success = parseVector(matrix, n.matrix);
		if (!success)
			return success;
	}
	if (rotation)
	{
		bool success = parseVector(rotation, n.rotation);
		if (!success)
			return success;
	}
	if (scale)
	{
		bool success = parseVector(scale, n.scale);
		if (!success)
			return success;
	}
	if (translation)
	{
		bool success = parseVector(translation, n.translation);
		if (!success)
			return success;
	}
	if (weights)
	{
		arrsetlen(n.weights, jsonArrayGetSize(weights));
		bool success = parseVector(weights, n.weights);
		if (!success)
			return success;
	}

	ctx->nodes[ix] = n;
	return true;
}

static void destroyNode(struct GLTFnode* n)
{
	arrfree(n->children);
	arrfree(n->weights);

	if (n->name)
		free(n->name);
}

#pragma endregion


#pragma region Sampler

struct SamplerReadCtx
{
	struct GLTFsampler* samplers
};
static _Success_(return != false) bool parseSamlperArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct SamplerReadCtx* ctx = (struct SamplerReadCtx*)userData;
	assert(ctx);
	JSONobject o = v.object;

	struct GLTFsampler s = {
		.magFilter = (GLenum)jsonObjectGetOrElse(o, "magFilter", _JVU(NULL)).uint,
		.minFilter = (GLenum)jsonObjectGetOrElse(o, "minFilter", _JVU(NULL)).uint,
		.wrapS = (GLenum)jsonObjectGetOrElse(o, "wrapS", _JVU(GL_REPEAT)).uint,
		.wrapT = (GLenum)jsonObjectGetOrElse(o, "wrapT", _JVU(GL_REPEAT)).uint,

		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),
	};
	ctx->samplers[ix] = s;
	return true;
}


#pragma endregion


#pragma region Scene

struct SceneReadCtx
{
	struct GLTFscene* scenes;

	const struct GLTFnode* nodes;
};
static _Success_(return != false) bool parseSceneArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct SceneReadCtx* ctx = (struct SceneReadCtx*)userData;
	assert(ctx != NULL);
	JSONobject o = v.object;

	struct GLTFscene s = {
		.nodes = NULL,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),
	};

	JSONarray nodes = jsonObjectGetOrElse(o, "nodes", _JVU(NULL)).array;
	if (nodes)
	{
		arrsetlen(s.nodes, jsonArrayGetSize(nodes));
		
		struct NodeChildrenReadCtx cCtx = { .children = s.nodes, .nodes = ctx->nodes };
		bool success = jsonArrayForeach(nodes, parseNodeChildrenArrayCallback, &ctx);
		if (!success)
			return false;
	}

	ctx->scenes[ix] = s;
	return true;
}

void destroyScene(struct GLTFscene* scene)
{
	if (scene->name)
		free(scene->name);
	arrfree(scene->nodes);
}

#pragma endregion


#pragma region Skin

struct SkinReadCtx
{
	struct GLTFskin* skins;

	const struct GLTFaccessor* accessors;
	const struct GLTFnode* nodes;
};
static _Success_(return != false) bool parseSkinArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct SkinReadCtx* ctx = (struct SkinReadCtx*)userData;
	assert(ctx != NULL);
	JSONobject o = v.object;

	struct GLTFskin s = {
		.inverseBindMatrices = jsonArrayGetType(o, "inverseBindMatrices") == JSONtype_INTEGER ? ctx->accessors + jsonObjectGet(o, "inverseBindMatrices").uint : NULL,
		.skeleton = jsonArrayGetType(o, "skeleton") == JSONtype_INTEGER ? ctx->nodes + jsonObjectGet(o, "skeleton").uint : NULL,
		.joints = NULL,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),
	};

	JSONarray joints = jsonObjectGet(o, "joints").array;
	if (joints)
	{
		arrsetlen(s.joints, jsonArrayGetSize(joints));
		struct NodeChildrenReadCtx cCtx = { .children = s.joints, .nodes = ctx->nodes };
		bool success = jsonArrayForeach(joints, parseNodeChildrenArrayCallback, &cCtx);
		if (success)
			return success;
	}

	ctx->skins[ix] = s;
	return true;
}

void destroySkin(struct GLTFskin* skin)
{
	if (skin->name)
		free(skin->name);
	arrfree(skin->joints);
}

#pragma endregion


#pragma region Texture

struct TextureReadCtx
{
	struct GLTFtexture* textures;

	const struct GLTFsampler* samplers;
	const struct GLTFimage* images;
};
static _Success_(return != false) bool parseTextureArrayCallback(_In_ JSONarray a, _In_ uint32_t ix, _In_ JSONtype t, _In_ JSONvalue v, _Inout_opt_ void* userData)
{
	struct TextureReadCtx* ctx = (struct TextureReadCtx*)userData;
	assert(ctx != NULL);
	JSONobject o = v.object;

	struct GLTFtexture tex = {
		.sampler = jsonObjectGetType(o, "sampler") == JSONtype_INTEGER ? ctx->samplers + jsonObjectGet(o, "sampler").uint : NULL,
		.source = jsonObjectGetType(o, "souce") == JSONtype_INTEGER ? ctx->images + jsonObjectGet(o, "source").uint : NULL,
		.name = copyStringHeap(jsonObjectGetOrElse(o, "name", _JVU(NULL)).string),
	};

	ctx->textures[ix] = tex;
	return true;
}

void destroyTexture(struct GLTFtexture* tex)
{
	if (tex->name)
		free(tex->name);
}

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

	itVersion = jsonObjectGetOrElse(asset, "minVersion", JVU(string = NULL)).string;
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
	if (bufferViews && buffers)
	{
		struct BufferViewReadCtx ctx = { .bufferViews = gltf.bufferViews, .buffers = gltf.buffers };
		jsonArrayForeach(bufferViews, parseBufferViewArrayCallback, &ctx);
	}
	if (cameras)
	{
		struct CameraReadCtx ctx = { .cameras = gltf.cameras };
		jsonArrayForeach(cameras, parseCameraArrayCallback, &ctx);
	}
	if (images)
	{
		struct ImageReadCtx ctx = { .images = gltf.images, .bufferViews = gltf.bufferViews };
		jsonArrayForeach(images, parseImageArrayCallback, &ctx);
	}
	if (materials)
	{
		struct MaterialReadCtx ctx = { .materials = gltf.materials, .textures = gltf.textures };
		jsonArrayForeach(materials, parseMaterialArrayCallback, &ctx);
	}
	if (meshes && accessors)
	{
		struct MeshReadCtx ctx = { .meshes = gltf.meshes, .accessors = gltf.accessors, .bufferViews = gltf.bufferViews, .materials = gltf.materials };
		jsonArrayForeach(meshes, parseMeshArrayCallback, &ctx);
	}
	if (nodes)
	{
		struct NodeReadCtx ctx = { .nodes = gltf.nodes, .cameras = gltf.cameras, .lights = gltf.lights, .meshes = gltf.meshes, .skins = gltf.skins };
		jsonArrayForeach(nodes, parseNodeArrayCallback, &ctx);
	}
	if (samplers)
	{
		struct SamplerReadCtx ctx = { .samplers = gltf.samplers };
		jsonArrayForeach(samplers, parseSamlperArrayCallback, &ctx);
	}
	if (scenes)
	{
		struct SceneReadCtx ctx = { .scenes = gltf.scenes, .nodes = gltf.nodes };
		jsonArrayForeach(scenes, parseSceneArrayCallback, &ctx);
	}
	if (skins)
	{
		struct SkinReadCtx ctx = { .skins = gltf.skins, .accessors = gltf.accessors, .nodes = gltf.nodes };
		jsonArrayForeach(skins, parseSkinArrayCallback, &ctx);
	}
	if (textures)
	{
		struct TextureReadCtx ctx = { .textures = gltf.textures, .samplers = gltf.samplers, .images = gltf.images };
		jsonArrayForeach(textures, parseTextureArrayCallback, &ctx);
	}

	if (scenes)
	{
		gltf.scene = jsonObjectGetType(object, "scene") == JSONtype_INTEGER ? gltf.scene + jsonObjectGet(object, "scene").uint : &gltf.scenes[0];
	}

	return gltf;
}

extern void gltfClear(_In_ struct GLTFgltf* gltf)
{
	const size_t accessorC = arrlenu(gltf->accessors);
	for (int i = 0; i < accessorC; i++)
		destroyAccessor(&gltf->accessors[i]);
	arrfree(gltf->accessors);

	const size_t animationC = arrlenu(gltf->animations);
	for (int i = 0; i < animationC; i++)
		destroyAnimation(&gltf->animations[i]);
	arrfree(gltf->animations);

	const size_t bufferC = arrlenu(gltf->buffers);
	for (int i = 0; i < bufferC; i++)
		destroyBuffer(&gltf->buffers[i]);
	arrfree(gltf->buffers);

	const size_t bufferViewC = arrlenu(gltf->bufferViews);
	for (int i = 0; i < bufferViewC; i++)
		destroyBufferView(&gltf->bufferViews[i]);
	arrfree(gltf->bufferViews);

	const size_t cameraC = arrlenu(gltf->cameras);
	for (int i = 0; i < cameraC; i++)
		destroyCamera(&gltf->cameras[i]);
	arrfree(gltf->cameras);

	const size_t imageC = arrlenu(gltf->images);
	for (int i = 0; i < imageC; i++)
		destroyImage(&gltf->images[i]);
	arrfree(gltf->images);

	const size_t materialC = arrlenu(gltf->materials);
	for (int i = 0; i < materialC; i++)
		destroyMaterial(&gltf->materials[i]);
	arrfree(gltf->materials);

	const size_t meshC = arrlenu(gltf->meshes);
	for (int i = 0; i < meshC; i++)
		destroyMesh(&gltf->meshes[i]);
	arrfree(gltf->meshes);

	const size_t nodeC = arrlenu(gltf->nodes);
	for (int i = 0; i < nodeC; i++)
		destroyNode(&gltf->nodes[i]);
	arrfree(gltf->nodes);

	const size_t samplerC = arrlenu(gltf->samplers);
	for (int i = 0; i < samplerC; i++)
		destroySampler(&gltf->samplers[i]);
	arrfree(gltf->samplers);

	const size_t sceneC = arrlenu(gltf->scenes);
	for (int i = 0; i < sceneC; i++)
		destroyScene(&gltf->scenes[i]);
	arrfree(gltf->scenes);

	const size_t skinC = arrlenu(gltf->skins);
	for (int i = 0; i < skinC; i++)
		destroySkin(&gltf->skins[i]);
	arrfree(gltf->skins);

	const size_t textureC = arrlenu(gltf->textures);
	for (int i = 0; i < textureC; i++)
		destroyTexture(&gltf->textures[i]);
	arrfree(gltf->textures);

	const size_t extensionsUsedC = arrlenu(gltf->extensionsUsed);
	for (int i = 0; i < extensionsUsedC; i++)
		free(&gltf->extensionsUsed[i]);
	arrfree(gltf->extensionsUsed);

	const size_t extensionsRequiredC = arrlenu(gltf->extensionsRequired);
	for (int i = 0; i < extensionsRequiredC; i++)
		free(&gltf->extensionsRequired[i]);
	arrfree(gltf->extensionsRequired);

	if (gltf->asset.copyright)
		free(gltf->asset.copyright);
	if (gltf->asset.generator)
		free(gltf->asset.generator);
	

}

extern void* gltfDataUriToBuffer(_In_z_ const char* data)
{
	const GLTFname
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