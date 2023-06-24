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


struct	GLTFaudioKHR;
struct	GLTFaudioEmitterKHR;
enum	GLTFaudioEmitter_typeKHR;
enum	GLTFaudioEmitter_distanceModel;
struct	GLTFaudioSourceKHR;


struct GLTFbuffer;


struct GLTFbufferView;


struct GLTFcamera;
struct GLTFcamera_orthographic;
struct GLTFcamera_perspective;


struct GLTFgltf;


struct GLTFimage;


struct GLTFlightKHR;
enum GLTFlight_typeKHR;


struct GLTFmaterial;
enum   GLTFmaterial_alphaMode;


struct GLTFmesh;
struct GLTFmesh_primitive;
struct GLTFmesh_primitive_attributesKV;
struct GLTFmesh_primitive_targetKV;
struct GLTFmesh_primitive_compressionDataKHR;
struct GLTFmesh_primitive_variantMappingsKHR;


enum   GLTFmimeType;

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
	
	float
		offset[2],
		rotation,
		scale[2];
};

struct GLTFvariantKHR;

#include <stdio.h>

extern struct GLTFgltf gltfRead(_In_ FILE* f,_In_ bool isGLB);

extern void gltfClear(_In_ struct GLTFgltf*);

extern GLTFname gltfMimeTypeToString(_In_ enum GLTFmimeType);


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
	union GLTFaccessor_minmax 
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
inline bool		_gltfAccessorHasValidSparse(_In_ struct GLTFaccessor* a)
{
	return a->sparse.count != 0;
}
inline uint32_t _gltfAccessorTypeComponentCount(_In_ enum GLTFaccessor_type type)
{
	switch (type)
	{
	case GLTFaccessor_type_SCALAR:  return 1;
	case GLTFaccessor_type_VEC2:	return 2;
	case GLTFaccessor_type_VEC3:    return 3;
	case GLTFaccessor_type_VEC4:
	case GLTFaccessor_type_MAT2:	return 4;
	case GLTFaccessor_type_MAT3:	return 9;
	case GLTFaccessor_type_MAT4:	return 16;

	default:						return 0;
	}
}
inline uint32_t _gltfComponentByteSize(_In_ GLenum type)
{
	switch (type)
	{
	case GL_UNSIGNED_BYTE:	case GL_BYTE:	return sizeof(char);
	case GL_UNSIGNED_SHORT: case GL_SHORT:	return sizeof(short);
	case GL_UNSIGNED_INT:	case GL_INT:	return sizeof(int);

	case GL_FLOAT: return sizeof(float);

	default: return 0;
	}
}


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

	GLTFanimation_channel_target_path_max,
};
struct GLTFanimation_sampler
{
	const struct GLTFaccessor
		* input,
		* output;
	enum GLTFanimation_sampler_interpolation interpolation;
};
enum   GLTFanimation_sampler_interpolation
{
	GLTFanimation_sampler_interpolation_LINEAR,
	GLTFanimation_sampler_interpolation_STEP,
	GLTFanimation_sampler_interpolation_CUBICSPLINE,
	GLTFanimation_sampler_interpolation_max,
};


struct	GLTFasset
{
	GLTFname copyright;
	GLTFname generator;
	uint32_t 
		versionMajor, 
		versionMinor,

		minVersionMajor,
		minVersionMinor;
};


struct	GLTFaudioKHR
{
	union {
		const struct GLTFbufferView* bufferView;
		GLTFname uri;
	};
	enum GLTFmimeType mimeType;
};
struct	GLTFaudioEmitterKHR
{
	GLTFname name;
	enum GLTFaudioEmitter_typeKHR type;
	float gain;
	const struct GLTFaudioSourceKHR** sources;

	float
		coneInnerAngle,
		coneOuterAngle,
		coneOuterGain,
		maxDistance,
		refDistance,
		rolloffFactor;
	enum GLTFaudioEmitter_distanceModel distanceModel;

};
enum	GLTFaudioEmitter_typeKHR
{
	GLTFaudioEmitter_type_positional,
	GLTFaudioEmitter_type_global,
};
enum	GLTFaudioEmitter_distanceModel
{
	GLTFaudioEmitter_distanceModel_linear,
	GLTFaudioEmitter_distanceModel_inverse,
	GLTFaudioEmitter_distanceModel_exponential,
};
struct	GLTFaudioSourceKHR
{
	GLTFname name;
	float gain;
	bool autoPlay;
	bool loop;
	const struct GLTFaudioKHR audio;
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
		struct GLTFcamera_orthographic orthographic;
		struct GLTFcamera_perspective  perspective;
	};
	bool isPerspective;
	GLTFname name;
};


struct GLTFgltf
{
	GLTFname					* extensionsUsed,
								* extensionsRequired;
	struct GLTFaccessor			* accessors;
	struct GLTFanimation		* animations;
	struct GLTFbuffer			* buffers;
	struct GLTFbufferView		* bufferViews;
	struct GLTFcamera			* cameras;
	struct GLTFimage			* images;
	struct GLTFmaterial			* materials;
	struct GLTFmesh				* meshes;
	struct GLTFnode				* nodes;
	struct GLTFsampler			* samplers;
	struct GLTFscene			* scenes;
	struct GLTFskin				* skins;
	struct GLTFtexture			* textures;

	struct GLTFlightKHR			* lights;
	struct GLTFvariantKHR		* variants;
	struct GLTFaudioKHR			* audio;
	struct GLTFaudioEmitterKHR	* emitters;
	struct GLTFaudioSourceKHR	* sources;

	struct GLTFasset asset;
	const struct GLTFscene* scene;
};


struct GLTFimage
{
	union {
		GLTFname uri;
		const struct GLTFbufferView* bufferView;
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
		alphaCutoff,
	// KHR_materials_anisotropy
		anisotropyStrength,
		anisotropyRotation,
	// KHR_materials_clearcoat
		clearcoatFactor,
		clearcoatRoughnessFactor,
		clearcoatNormalScale,
	// KHR_materials_diffuse_transmission
		diffuseTransmissionFactor,
		diffuseTransmissionColorFactor[3],
	// KHR_materials_emissive_strength
		emissiveStrength,
	// KHR_materials_ior
		ior,
	// KHR_materials_iridescence
		iridescenceFactor,
		iridescenceIor,
		iridescenceThicknessMinimum,
		iridescenceThicknessMaximum,
	// KHR_materials_sheen
		sheenColorFactor[3],
		sheenRoughnessFactor,
	// KHR_materials_specular
		specularFactor,
		specularColorFactor[3],
	// KHR_materials_sss
		scatterDistance,
		scatterColor[3],
	// KHR_materials_transmission
		transmissionFactor,
	// KHR_materials_volume
		thicknessFactor,
		attenuationDistance,
		attenuationColor[3]
		;

	enum GLTFmaterial_alphaMode alphaMode;
	bool doubleSided;
	bool unlit;

	struct GLTFtextureInfo
		baseColorTexture,
		metallicRoughnessFactor,
		normalTexture,
		occlusionTexture,
		emissiveTexture,
	// KHR_materials_anisotropy
		anisotropyTexture,
	// KHR_materials_clearcoat
		clearcoatTexture,
		clearcoatRoughnessTexture,
		clearcoatNormalTexture,
	// KHR_materials_diffuse_transmission
		diffuseTransmissionTexture,
		diffuseTransmissionColorTexture,
	// KHR_materials_iridescence
		iridescenceTexture,
		iridescenceThicknessTexture,
	// KHR_materials_sheen
		sheenColorTexture,
		sheenRoughnessTexture,
	// KHR_materials_specular
		specularTexture,
		specularColorTexture,
	// KHR_materials_transmission
		transmissionTexture,
	// KHR_materials_volume
		thicknessTexture
		;
};
enum   GLTFmaterial_alphaMode
{
	GLTFmaterial_alphaMode_OPAQUE,
	GLTFmaterial_alphaMode_MASK,
	GLTFmaterial_alphaMode_BLEND,

	GLTFmaterial_alphaMode_max,
};


struct GLTFmesh
{
	struct GLTFmesh_primitive* primitives;
	float* weights;

	GLTFname name;
};
struct GLTFmesh_primitive_compressionDataKHR
{
	const struct GLTFbufferView* bufferView;
	struct GLTFmesh_primitive_attributesKV* attributes;
};
struct GLTFmesh_primitive
{
	struct GLTFmesh_primitive_attributesKV* attributes;
	const struct GLTFaccessor* indices;
	const struct GLTFmaterial* material;
	struct GLTFmesh_primitive_targetKV** targets;
	GLenum mode;

	struct GLTFmesh_primitive_compressionDataKHR compressionData;
};
struct GLTFmesh_primitive_attributesKV
{
	char* key;
	const struct GLTFaccessor* value;
};
struct GLTFmesh_primitive_targetKV
{
	char* key;
	const struct GLTFaccessor* value;
};

struct GLTFmesh_primitive_variantMappingsKHR 
{
	const struct GLTFmaterial* material;
	const struct GLTFvariantKHR** variants;
};


struct GLTFnode
{
	const struct GLTFcamera* camera;
	struct GLTFnode** children;
	const struct GLTFskin* skin;
	const struct GLTFmesh* mesh;

	float
		matrix[16],
		rotation[4],
		scale[3],
		translation[3],
		* weights;

	GLTFname name;

	const struct GLTFlightKHR* light;
	const struct GLTFaudioEmitterKHR* emitter;
};


enum   GLTFmimeType
{
	GLTFmimeType_NONE,

	GLTFmimeType_AUDIO_MPEG,

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

	const struct GLTFaudioEmitterKHR** emitters;
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


struct GLTFlightKHR
{
	float
		color[3],
		intensity,
		range;
	enum GLTFlights_typeKHR type;

	GLTFname name;
};
enum GLTFlight_typeKHR
{
	GLTFlight_type_point,
	GLTFlight_type_spot,
	GLTFlight_type_directional,
};


struct GLTFvariantKHR
{
	GLTFname name;
};