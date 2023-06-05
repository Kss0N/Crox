#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stb_ds.h>

#ifndef __gl_h_

typedef uint32_t GLenum;

#endif // __gl_h_


struct GLTFaccessor;
union  GLTFaccessor_minmax;
struct GLTFaccessor_sparse;
enum   GLTFaccessor_type;


struct GLTFanimation;
struct GLTFanimation_channel;
enum   GLTFanimation_channel_target_path;
struct GLTFanimation_sampler;
enum   GLTFanimation_sampler_interpolation;


struct GLTFasset;


struct GLTFbuffer;


struct GLTFbufferView;


struct GLTFcamera;
struct GLTFcamera_orthographic;
struct GLTFcamera_perspective;


struct GLTFgltf;


struct GLTFimage;


struct GLTFmaterial;
enum   GLTFmaterial_alphaMode;


struct GLTFmesh;
struct GLTFmesh_primitive;
struct GLTFmesh_primitive_attributesKV;
struct GLTFmesh_primitive_targetsKV;


enum   GLTFmimetType;

typedef _Null_terminated_ const char* GLTFname;


struct GLTFnode;


struct GLTFsampler;


struct GLTFscene;


struct GLTFskin;


struct GLTFtexture;
struct GLTFtextureInfo
{
	const struct GLTFtextureInfo* texture;
	uint32_t texCoord;
};

#include <stdio.h>

extern struct GLTFgltf gltfRead(_In_ FILE* f,_In_ bool isGLB);

extern void gltfClear(_In_ struct GLTFgltf*);


struct GLTFaccessor_sparse
{
	uint32_t count;
	const struct GLFTbufferView
		*indices_bufferView,
		* values_bufferView;
	uint32_t
		indices_byteOffset,
		 values_byteOffset;
	GLenum indices_componentType;
};
union  GLTFaccessor_minmax
{
	uint8_t		i8;
	int8_t		u8;
	int16_t		i16;
	uint16_t	u16;
	int32_t		i32;
	uint32_t	u32;
	float		f32;
};
struct GLTFaccessor
{
	const struct GLTFbufferView* bufferView;
	uint32_t byteOffset;
	GLenum componenType;
	bool normalized;
	uint32_t count;
	enum GLTFaccessor_type type;
	GLTFaccessor_minmax 
		max[16],
		min[16];
	struct GLTFaccessor_sparse sparse;

	GLTFname name;
};
enum   GLTFaccessor_type
{
	GLTFaccessor_type_SCALAR,
	
	GLTFaccessor_type_VEC2,
	GLTFaccessor_type_VEC3,
	GLTFaccessor_type_VEC4,

	GLTFaccessor_type_MAT2,
	GLTFaccessor_type_MAT3,
	GLTFaccessor_type_MAT4,
};


struct GLTFanimation
{
	struct GLTFanimation_channel* channels;
	struct GLTFanimation_sampler* samplers;
	GLTFname name;
};
struct GLTFanimation_channel
{
	const struct GLTFanimation_sampler* sampler;

	const struct GLTFnode* target_node;
	enum GLTFanimation_channel_target_path target_path;
};
enum   GLTFanimation_channel_target_path
{
	GLTFanimation_channel_target_path_TRANSLATION,
	GLTFanimation_channel_target_path_ROTATION,
	GLTFanimation_channel_target_path_SCALE,
	GLTFanimation_channel_target_path_WEIGHTS,
};
struct GLTFanimation_sampler
{
	const GLTFaccessor
		* input,
		* output;
	enum GLTFanimation_sampler_interpolation interpolation;
};
enum   GLTFanimation_sampler_interpolation
{
	GLTFanimation_sampler_interpolation_LINEAR,
	GLTFanimation_sampler_interpolation_STEP,
	GLTFanimation_sampler_interpolation_CUBICSPLINE,
};


struct GLTFasset
{
	GLTFname copyright;
	GLTFname generator;
	GLTFname version;
	GLTFname minVersion;
};


struct GLTFbuffer
{
	union {
		GLTFname uri;
		void* data;
	};
	size_t byteLength;
	GLTFname name;
};


struct GLTFbufferView
{
	const struct GLTFbuffer* buffer;
	uint32_t byteOffset;
	uint32_t byteLength;
	uint32_t byteStride;
	GLenum target;
	GLTFname name;
};


struct GLTFcamera_orthographic
{
	float
		xmag,
		ymag,
		zfar,
		znear;
};
struct GLTFcamera_perspective
{
	float
		aspectRatio,
		yfov,
		zfar,
		znear;
};
struct GLTFcamera
{
	union 
	{
		GLTFcamera_orthographic orthographic;
		GLTFcamera_perspective  perspective;
	};
	bool isPerspective;
};


struct GLTFgltf
{
	GLTFname				* extensionsUsed,
							* extensionsRequired;
	struct GLTFaccessor		* accessors;
	struct GLTFanimation	* animations;
	struct GLTFbuffer		* buffers;
	struct GLTFbufferView	* bufferView;
	struct GLTFcamera		* cameras;
	struct GLTFimage		* images;
	struct GLTFmaterial		* materials;
	struct GLTFmesh			* mesh;
	struct GLTFnode			* nodes;
	struct GLTFsampler		* sampler;
	struct GLTFscene		* scenes;
	struct GLTFskin			* skins;
	struct GLTFtexture		* textures;

	GLTFasset asset;
	const struct GLTFscene* scene;
};


struct GLTFimage
{
	union {
		GLTFname uri;
		const struct GLTFbufferView bufferView;
	};
	enum GLTFmimeType mimeType;
	GLTFname name;
};


struct GLTFmaterial
{
	GLTFname name;
	
	float
		baseColorFactor[4],
		metallicFactor,
		roughnessFactor,
		normalScale,
		occlusionStrength,
		emissiveFactor[3],
		alphaCutoff;

	bool doubleSided;
	enum GLTFmaterial_alphaMode alphaMode;

	struct GLTFtextureInfo
		baseColorTexture,
		metallicRoughnessFactor,
		normalTexture,
		occlusionTexture,
		emissiveTexture;
};
enum   GLTFmaterial_alphaMode
{
	GLTFmaterial_alphaMode_OPAQUE,
	GLTFmaterial_alphaMode_MASK,
	GLTFmaterial_alphaMode_BLEND,
};


struct GLTFmesh
{
	struct GLTFmesh_primitive* primitives;
	float* weights;

	GLTFname name;
};
struct GLTFmesh_primitive
{
	struct GLTFmesh_primitive_attributesKV* attributes;
	const struct GLTFaccessor* indices;
	const struct GLTFmaterial* material;
	struct GLTFmesh_primitive_targetsKV* targets;
	GLenum mode;
};
struct GLTFmesh_primitive_attributesKV
{
	char* key;
	const struct GLTFaccessor* value;
};
struct GLTFmesh_primitive_targetsKV
{
	char* key;
	const struct GLTFaccessor* value;
};


struct GLTFnode
{
	const struct GLTFcamera* camera;
	struct GLTFnode* children;
	const struct GLTFskin* skin;
	const struct GLTFmesh* mesh;

	float
		matrix[16],
		rotation[4],
		scale[3],
		translation[3],
		* weights;

	GLTFname name;
};


enum   GLTFmimeType
{
	GLTFmimeType_IMAGE_JPEG,
	GLTFmimeType_IMAGE_PNG,
};


struct GLTFsampler
{
	GLenum
		magFilter,
		minFilter,
		wrapS,
		wrapT;
	GLTFname name;
};


struct GLTFscene
{
	struct GLTFnode* nodes;
	GLTFname name;
};


struct GLTFskin
{
	const struct GLTFaccessor* inverseBindMatrices;
	const struct GLTFnode* skeleton;
	const struct GLTFnode** joints;
	GLTFname name;
};


struct GLTFtexture
{
	const struct GLTFsampler* sampler;
	const struct GLTFimage* source;
	GLTFname name;
};
