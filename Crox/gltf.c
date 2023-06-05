#include <stdint.h>
#include <stdbool.h>
#include <glad/gl.h>


struct GltfAccessor;
enum   GltfAccessorType;
struct GltfBuffer;
struct GltfBufferView;


enum GltfAccessorType
{
	GLTF_ACCESSOR_TYPE_SCALAR,
	GLTF_ACCESSOR_TYPE_VEC2,
	GLTF_ACCESSOR_TYPE_VEC3,
	GLTF_ACCESSOR_TYPE_VEC4,
	GTLF_ACCESSOR_TYPE_MAT2,
	GTLF_ACCESSOR_TYPE_MAT3,
	GLTF_ACCESSOR_TYPE_MAT4,
};


struct GltfAccessor
{
	struct GltfBuffer* bufferView;
	uint32_t byteOffset;
	uint32_t componentType;
	bool normalized;
	uint32_t count;
	enum GltfAccessorType type;

	void* max; 
	void* min;

};

struct GltfBuffer
{
	uint32_t flags;
	uint32_t byteLength;
	void* data;
	const char* name;
};

struct GltfBufferView
{
	struct GltfBuffer* buffer;

	uint32_t byteOffset;
	uint32_t byteLength;
	uint32_t byteStride;
	uint32_t target; // GL_ARRAY_BUFFER | GL_ELEMENT_ARRAY_BUFFER

	const char* name;
};

struct GltfImage
{
	const char* mimeType;
	void* data;
	const char* name;
};




struct GltfData
{
	uint32_t unused;
};
