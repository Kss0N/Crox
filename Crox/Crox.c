/**

	@file      Crox.c
	@brief     Entry point for the Crox Application Layer.
	@details   This is a state containing module.
	@author    Jakob Kristersson <jakob.kristerrson@bredband.net> [Kss0N]
	@date      17.04.2023

**/
#include "Crox.h"
#include "platform/Platform.h"
#include "framework_crt.h"
#include "framework_spirv.h"
#include "mesh.h"
#include "wavefront.h"
#include "camera.h"
#include "log.h"

#include <glad/gl.h>
#include <glad/wgl.h>
#include <linmath.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <stb_include.h>
#include <cwalk.h>

#include "gltf.h"
#include "json.h"


#define _USE_MATH_DEFINES
#include <math.h>
#include <tchar.h>

#define NAME_OBJECT(type, obj, name) glObjectLabel(type, obj, -(signed)strlen(name),name);

typedef _Null_terminated_ const char* Path;

inline const int32_t getDriverConstant(GLenum property)
{
	int32_t a;
	glGetIntegerv(property, &a);
	return a;
}

static const uint8_t* readBinary(_In_z_ Path p, _Out_ size_t* len)
{
	FILE* f;
	fopen_s(&f, p, "rb");
	if (!f)
	{
		cxERROR(_T("Could not open path <%hs>\n"), p);
		*len = 0;
		return NULL;
	}

	fseek(f, 0, SEEK_END);

	*len = (size_t) ftell(f);
	fseek(f, 0, SEEK_SET);
	uint8_t* data = malloc(*len+1);
	assert(data != NULL); //TODO

	
	size_t readCount = fread_s(data, *len, 1, *len, f);
	assert(readCount == *len);

	fclose(f);
	return data;
}

inline void cleanupSource(_In_opt_ const char* src)
{
	if (src != NULL)
		free((void*)src);
}

inline void detachDeleteShader(_In_ GLuint fromProgram, _In_ GLuint shader)
{
	if (shader) 
	{
		glDetachShader(fromProgram, shader);
		glDeleteShader(shader);
	}
}

#ifdef _DEBUG
inline bool checkCompileStatus(_In_ GLuint shader) 
{
	GLint isCompiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
#ifndef glDebugMessageCallback
	if (!isCompiled)
	{
		GLint msgLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msgLen);
		
		GLchar* msg = alloca(msgLen+1);
		glGetShaderInfoLog(shader, msgLen, NULL, msg);

		cxERROR(_T("Error compiling shader: %hs"), msg);
	}
#endif // !glDebugMessageCallback
	return isCompiled;
}
inline bool isSPIRV(_In_ GLuint shader)
{
	GLint isSpirv = false;
	glGetShaderiv(shader, GL_SPIR_V_BINARY, &isSpirv);
	return isSpirv;
}
#else
#define checkCompileStatus(shader)
#define isSPIRV(shader)
#endif // _DEBUG

static void compileAndAttachSPIRV(_In_ GLuint shader, _In_ GLuint program, _In_ size_t codeLength, _In_reads_bytes_(codeLength) const GLchar* code)
{
	glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, code, (GLsizei)codeLength);
	assert(isSPIRV(shader));
	glSpecializeShader(shader, "main", 0, NULL, NULL);
	assert(checkCompileStatus(shader));
	glAttachShader(program, shader);
}
static inline bool XOR(bool p, bool q)
{
	return (p || q) && !(p && q);
}

/**
	@brief  Creates a OpenGL program using SPIR-V sources. Sources must have "main" as entry point.
	@param  vertex   - path to vertex shader SPIR-V 
	@param  tessEval - path to tesselation evaluation shader SPIR-V. If the tessEval stage is skipped, this parameter is NULL, though SHOULD be followed by tessEval.
	@param  tessCtrl - path to tesselation control shader SPIR-V. If the tessCtrl stage is skipped, this parameter is NULL, though SHOULD be following tessCtrl.
	@param  geometry - path to geometry shader SPIR-V. If the geometry stage is skipped, this parameter is NULL
	@param  fragment - path to fragment shader SPIR-V.
	@retval          - valid program. 0 on failure
**/
_Must_inspect_result_ _Success_(return != 0) static GLuint 
makeProgramSPIRV(
	_In_z_		Path		  vertex,
	_In_opt_z_	Path		tessEval,
	_In_opt_z_	Path		tessCtrl,
	_In_opt_z_	Path		geometry,
	_In_z_		Path		fragment)
{
	assert(vertex != NULL);
	assert(fragment != NULL);

#ifdef _DEBUG
	GLint spirvSupport = GL_FALSE;
	glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &spirvSupport);
	if (!spirvSupport)
	{
		cxFATAL(_T("The Vendor does NOT support SPIR-V"));
		return 0;
	}
	assert(spirvSupport != GL_FALSE);

	if (tessEval && !tessCtrl)
		cxWARN(_T("Creating program with tesselation evaluation without control.\n"));
#endif // _DEBUG	

	GLuint program = 0;

	size_t 
		 vSrcLen = 0,
		tcSrcLen = 0,
		teSrcLen = 0,
		 gSrcLen = 0,
		 fSrcLen = 0;

	const uint8_t
		* vSrc = readBinary( vertex, &vSrcLen),						//	   Vertex SPIRV Source
		*teSrc = tessEval ? readBinary(tessEval, &tcSrcLen) : NULL,	//	Tess Eval SPIRV source
		*tcSrc = tessCtrl ? readBinary(tessCtrl, &teSrcLen) : NULL,	//	Tess Ctrl SPIRV source
		* gSrc = geometry ? readBinary(geometry, & gSrcLen) : NULL,	//   Geometry SPIRV Source
		* fSrc = readBinary(fragment, &fSrcLen);					//   fragment SPIRV Source
	
	//
	// GOTOs are generally avoided. personally I make exceptions when it comes to error handling
	//

	if (vSrc != NULL &&
		!XOR(tessEval != NULL,teSrc != NULL) &&
		!XOR(tessCtrl != NULL,tcSrc != NULL) &&
		!XOR(geometry != NULL, gSrc != NULL) &&
		fSrc != NULL)
		goto CREATE_SHADERS; 

	// Failure to load: log errors and cleanup.

	if (vSrc == NULL)
		cxERROR(_T("Could not read vertex Shader SPIR-V <%hs>\n"), vertex);
	if (tessCtrl && tcSrc == NULL)
		cxERROR(_T("Could not read tesselation control Shader SPIR-V <%hs>\n"), tessCtrl);
	if (tessEval && teSrc == NULL)
		cxERROR(_T("Could not read tesselation evaluation Shader SPIR-V <%hs>\n"), tessEval);
	if (geometry &&  gSrc == NULL)
		cxERROR(_T("Could not read geometry shader SPIR-V <%hs>\n"), geometry);
	if (fSrc == NULL)
		cxERROR(_T("Could not not read fragment shader SPIR-V <%hs>\n"), fragment);

	goto CLEANUP_SOURCE;
CREATE_SHADERS:
	GLuint
		 vShader = glCreateShader(GL_VERTEX_SHADER),					// Vertex shader
		teShader = teSrc ? glCreateShader(GL_TESS_EVALUATION_SHADER): 0,// Tesselation evalutation shader (optional)
		tcShader = tcSrc ? glCreateShader(GL_TESS_CONTROL_SHADER)	: 0,// Tesselation control shader (optional)
		 gShader =  gSrc ? glCreateShader(GL_GEOMETRY_SHADER)		: 0,// Geometry shader (optional)
		 fShader = glCreateShader(GL_FRAGMENT_SHADER);					// Fragment shader		

#ifdef _DEBUG
	NAME_OBJECT(GL_SHADER, vShader, vertex);
	if (teShader) NAME_OBJECT(GL_SHADER, teShader, tessEval);
	if (tcShader) NAME_OBJECT(GL_SHADER, tcShader, tessCtrl);
	if ( gShader) NAME_OBJECT(GL_SHADER,  gShader, geometry);
	NAME_OBJECT(GL_SHADER, fShader, fragment);
#endif // _DEBUG

	program = glCreateProgram();

	compileAndAttachSPIRV(vShader, program, vSrcLen, vSrc);
	if (teShader) compileAndAttachSPIRV(teShader, program, teSrcLen, teSrc);
	if (tcShader) compileAndAttachSPIRV(tcShader, program, tcSrcLen, tcSrc);
	if ( gShader) compileAndAttachSPIRV( gShader, program,  gSrcLen,  gSrc);
	compileAndAttachSPIRV(fShader, program, fSrcLen, fSrc);
	
	glLinkProgram(program);
	
	GLint isLinked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	if (!isLinked)
	{
		glDeleteProgram(program);
		program = 0;
	}

	detachDeleteShader(program, vShader);
	detachDeleteShader(program,teShader);
	detachDeleteShader(program,tcShader);
	detachDeleteShader(program, gShader);
	detachDeleteShader(program, fShader);

CLEANUP_SOURCE:
	cleanupSource(vSrc);
	cleanupSource(teSrc);
	cleanupSource(tcSrc);
	cleanupSource(teSrc);
	cleanupSource(gSrc);
	cleanupSource(fSrc);

	return program;
}


struct TexInfo {
	float
		xOffset, yOffset,
		width, height;
};
_Static_assert(sizeof(struct TexInfo) == sizeof(vec4), "make sure that texinfo and vec4 are the same type as the texInfo will be 'translated' into vec4 when being transfered from cpu to gpu \n");

static void uploadToMaterialBuffer(uint8_t* buf, 
	_In_ vec3 kAmbient, 
	_In_ vec3 kDiffuse, 
	_In_ vec3 kSpecular, 
	_In_ float nAlpha, 
	_In_ float nShininess,
	_In_opt_ struct TexInfo* mAmbient,
	_In_opt_ struct TexInfo* mDiffuse,
	_In_opt_ struct TexInfo* mSpecular,
	_In_opt_ struct TexInfo* mNormal,
	_In_opt_ struct TexInfo* mAlpha,
	_In_opt_ struct TexInfo* mShininess)
{
	size_t offset = 0;

	//u_ambientColor
	memcpy(buf + offset, kAmbient, sizeof(vec3));
	offset += sizeof(vec3);

	//u_alpha
	memcpy(buf + offset, &nAlpha, sizeof(float));
	offset += sizeof(float);

	//u_diffuseColor
	memcpy(buf + offset, kDiffuse, sizeof(vec3));
	offset += sizeof(vec3);

	//u_shininess
	memcpy(buf + offset, &nShininess, sizeof(float));
	offset += sizeof(float);

	//u_specularColor
	memcpy(buf + offset, kSpecular, sizeof(vec3));
	offset += sizeof(vec3);

	//unused
	offset += sizeof(float);

	//ambient map
	if (mAmbient)
		memcpy(buf + offset, mAmbient, sizeof(struct TexInfo));
	else
		memset(buf + offset, 0, sizeof(struct TexInfo));
	offset += sizeof(struct TexInfo);

	//diffuse map
	if (mDiffuse)
		memcpy(buf + offset, mDiffuse, sizeof(struct TexInfo));
	else
		memset(buf + offset, 0, sizeof(struct TexInfo));
	offset += sizeof(struct TexInfo);

	//specular map
	if (mSpecular)
		memcpy(buf + offset, mSpecular, sizeof(struct TexInfo));
	else
		memset(buf + offset, 0, sizeof(struct TexInfo));
	offset += sizeof(struct TexInfo);

	//normal map
	if (mNormal)
		memcpy(buf + offset, mNormal, sizeof(struct TexInfo));
	else
		memset(buf + offset, 0, sizeof(struct TexInfo));
	offset += sizeof(struct TexInfo);

	//alpha map
	if (mAlpha)
		memcpy(buf + offset, mAlpha, sizeof(struct TexInfo));
	else
		memset(buf + offset, 0, sizeof(struct TexInfo));
	offset += sizeof(struct TexInfo);

	//shininess map
	if (mShininess)
		memcpy(buf + offset, mShininess, sizeof(struct TexInfo));
	else
		memset(buf + offset, 0, sizeof(struct TexInfo));
	offset += sizeof(struct TexInfo);	
}

static bool parseMap(_In_ const char* path, _Out_ stbi_uc** pImage, _Out_ struct TexInfo* pInfo)
{
	struct TexInfo info = {0,0,0,0};

	int width, height, eChannelLayout;
	stbi_uc* img = stbi_load(path, &width, &height, &eChannelLayout, STBI_rgb_alpha);
	if (img)
	{
		info.width = (float)width;
		info.height = (float)height;
		*pImage = img;
		*pInfo = info;
		return true;
	}
	*pImage = NULL;
	*pInfo = info;
	return false;
}

// when the there are less than 2^16 vertices, the indices should be of type uint16_t on the GPU side, as to half the memory footprint.
static size_t indicesByteSize(size_t vertexCount, size_t indexCount)
{
	return vertexCount < UINT16_MAX
		? indexCount * sizeof(uint16_t) + 2 * (indexCount % 2)
		: indexCount * sizeof(uint32_t)
		;
}
static uint32_t ceilTo(uint32_t x, uint32_t to)
{
	return to * (uint32_t)(x / to) + to;
}


struct group {
	uint32_t 
		ixFirst,
		count;

	 int32_t ixMaterial;
	uint32_t materialSize;
};

struct mesh
{
	struct group* groups;	//stb_ds darray
	LPCSTR name;


	uint32_t vertexCount;	//count of vertices
	uint32_t vertexOffset;	//count of preceeding vertices in batch
	uint32_t indexOffset;	//byte Offset into index batch

	GLuint vao;
	GLuint program;

	mat4 matrix;
	
};
void destroy_mesh(_Inout_ struct mesh* m)
{
	arrfree(m->groups);
	
	if (m->name) 
	{
		_aligned_free((void*)m->name);
		m->name = NULL;
	}
	m->vertexCount = 0;
	m->vertexOffset = 0;
}

_Check_return_ _Success_(return != NULL)
/**
	@brief  Creates a stb_ds darray of meshes read from the paths. All vertex, index and material data will be stored sequencialy in the vbo, ebo and ubo supplied as parameter. The Factory thus expects them to be 1) newly created and b) not contain any data
	@param  count - count of paths
	@param  paths - paths to wavefront obj files (file extension MUST be .obj)
	@param  vbo   - newly created VBO to fill with vertices	data.
	@param  ebo   - newly created EBO to fill with indices	data
	@param  ubo   - newly created UBO to fill with materials data
	@param  atlas - newly created tex2D to fill with images for materials 
	@retval       - stb_ds darray of meshes, size being `count`.
**/
inline struct mesh* createMeshes(
	_In_				uint32_t	count, 
	_In_reads_(count)	const Path*	paths, 
	_Inout_				GLuint		vbo, 
	_Inout_				GLuint		ebo, 
	_Inout_				GLuint		ubo,
	_Inout_				GLuint		atlas)
{
	// Returns
	struct mesh* meshes = malloc(count * sizeof(struct mesh));
	if (!meshes) return NULL;

	//
	// Utilities
	//

	struct Material 
	{
		vec3
			ambient,
			diffuse,
			specular;
		float
			alpha,
			shininess;

		// -1 means that the material does not use a map for that property
		int32_t
			ixMapAmbient,
			ixMapDiffuse,
			ixMapSpecular,
			ixMapNormal,
			//ixMapAlphaShininess; //Since the alpha map will only use the A channel, the Shinniness map could be appended ontop of it, using the R channel, thus saving space from reduncandies
			ixMapAlpha,
			ixMapShininess;
	};

	uint32_t 
		vertexCount = 0,
	      indexSize = 0;
	struct Vertex	**verticesV = NULL;	// stb_ds Vector of vertex arrays
	uint32_t		**indicesV  = NULL;	// stb_ds Vector of element arrays
	struct Material	* materials = NULL;	// stb_ds Vector of material buffers
	struct TexInfo	* infos		= NULL; // stb_ds Vector of material maps.
	stbi_uc			**images	= NULL; // stb_ds Vector of map images

	struct WavefrontObj obj;
	struct WavefrontMtllibKV* mtllib = NULL;

	char
		* mtllibPath = _aligned_malloc(MAX_PATH, 8),
		* imagePath = _aligned_malloc(MAX_PATH, 8);

	const int32_t UBO_ALIGNMENT = getDriverConstant(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);

	stbi_set_flip_vertically_on_load(true);
	
	Path path;
	for (uint32_t ix = 0;
		ix < count;
		++ix, wfDestroyObj(&obj), wfDestroyMtllib(&mtllib))
	{
		path = paths[ix];
		FILE* file;
		errno_t err;  

		struct mesh* active = meshes + ix;
		memset(active, 0, sizeof(struct mesh));
		
		err = fopen_s(&file, path, "r");
		if (err != 0)
			goto CLEANUP_INVALID;
		obj = wavefront_obj_read(file);
		assert(obj.vertices != NULL);
		fclose(file);

		if (obj.mtllib)
		{
			cwk_path_change_basename(path, obj.mtllib, mtllibPath, MAX_PATH);
			err = fopen_s(&file, mtllibPath, "r");
			if (err != 0)
				goto CLEANUP_INVALID;
			mtllib = wavefront_mtl_read(file);
			assert(shlenu(mtllib) > 0);
			fclose(file);
		}
		if (obj.name)
		{
			//Let's be a bit clever and just transfer ownership.
			active->name = obj.name;
			obj.name = NULL;
		}
		
		cxINFO(_T("Creating mesh \"%hs\".\n"), active->name ? active->name : "NONAME");

		const uint32_t 
			mtlCount  = mtllib ? (uint32_t)shlenu(mtllib) : 0,
			mtlOffset = (uint32_t)arrlenu(materials);

		if (mtlCount > 0)
			arrsetlen(materials, arrlenu(materials) + mtlCount);
			
		const uint32_t groupCount = (uint32_t)arrlenu(obj.groups);
		for (uint32_t ixG = 0; ixG < groupCount; ixG++)
		{
			struct WavefrontGroup* group = obj.groups + ixG;
			
			struct group g = {
				.ixFirst	= group->ixFirst,
				.count		= group->count,
				.ixMaterial = -1,
			};

			//
			// Parse material. If Group has no material, use some default material.
			//
			
			const uint32_t ixMtl = (uint32_t)shgeti(mtllib, group->mtlName);
			if (ixMtl == -1) 
				goto SKIP_MATERIAL;
			const struct WavefrontMtl* pM = &shgetp(mtllib, group->mtlName)->value;
			cxINFO(_T("\tParsing Material \"%hs\"\n"), group->mtlName);

			//NOTE: materials are specified per the uniform `MaterialBlock` in file `3D.frag`
			//TODO: make this more "shader agnostic"

			//TODO: merge alpha and shininess mapc
			stbi_uc
				* ambientMap		= NULL,
				* diffuseMap		= NULL,
				* specularMap		= NULL,
				* normalMap			= NULL,
				* alphaShininessMap = NULL, //TODO
				* alphaMap			= NULL,
				* shininessMap		= NULL;
			struct TexInfo
				ambientInfo			= { 0 },
				diffuseInfo			= { 0 },
				specularInfo		= { 0 },
				normalInfo			= { 0 },
				alphaShininessInfo	= { 0 }, //TODO
				alphaInfo			= { 0 },
				shininessInfo		= { 0 };


			if (pM->ambientMapPath)
			{
				cwk_path_change_basename(path, pM->ambientMapPath, imagePath, MAX_PATH);
				if (!parseMap(imagePath, &ambientMap, &ambientInfo))
					goto CLEANUP_INVALID;
			}
			if (pM->diffuseMapPath)
			{
				cwk_path_change_basename(path, pM->diffuseMapPath, imagePath, MAX_PATH);
				if (!parseMap(imagePath, &diffuseMap, &diffuseInfo))
					goto CLEANUP_INVALID;
			}
			if (pM->specularMapPath)
			{
				cwk_path_change_basename(path, pM->specularMapPath, imagePath, MAX_PATH);
				if (!parseMap(imagePath, &specularMap, &specularInfo))
					goto CLEANUP_INVALID;
			}
			if (pM->bumpMapPath)
			{
				cwk_path_change_basename(path, pM->bumpMapPath, imagePath, MAX_PATH);
				if (!parseMap(imagePath, &normalMap, &normalInfo))
					goto CLEANUP_INVALID;
			}

			if (pM->alphaMapPath || pM->shininessMapPath)
			{
				if (pM->alphaMapPath)
				{
					cwk_path_change_basename(path, pM->alphaMapPath, imagePath, MAX_PATH);
					if (!parseMap(imagePath, &alphaMap, &alphaInfo))
						goto CLEANUP_INVALID;
				}
				if (pM->shininessMapPath)
				{
					cwk_path_change_basename(path, pM->shininessMapPath, imagePath, MAX_PATH);
					struct TexInfo info;
					stbi_uc* image;
					if (!parseMap(imagePath, &shininessMap, &shininessInfo))
						goto CLEANUP_INVALID;
				}

				//alpha, but not shininess
				if (alphaMap && !shininessMap)
				{
					alphaShininessMap = alphaMap;
					alphaShininessInfo = alphaInfo;
					goto SKIP_MERGE;
				}

				//shininess, but not alpha
				if (shininessMap && !alphaMap)
				{
					alphaShininessMap = shininessMap;
					alphaShininessInfo = shininessInfo;
					goto SKIP_MERGE;
				}

				//
				//alpha and shininess => merge both images into one.
				//

				//TODO: merge images

			SKIP_MERGE:
				;
			}

			struct Material mat = { 0 };
			vec3_dup(mat.ambient, pM->ambient);
			vec3_dup(mat.diffuse, pM->diffuse);
			vec3_dup(mat.specular,pM->specular);
			mat.alpha = pM->alpha;
			mat.shininess = pM->shininess;

			mat.ixMapAmbient	= -1;
			mat.ixMapDiffuse	= -1;
			mat.ixMapSpecular	= -1;
			mat.ixMapNormal		= -1;
			mat.ixMapAlpha		= -1;
			mat.ixMapShininess	= -1;

			if (ambientMap) 
			{
				mat.ixMapAmbient = arrlen(images);
				arrput(images, ambientMap);
				arrput(infos, ambientInfo);
			}
			if (diffuseMap)
			{
				mat.ixMapDiffuse = arrlen(images);
				arrput(images, diffuseMap);
				arrput(infos, diffuseInfo);
			}
			if (specularMap)
			{
				mat.ixMapSpecular = arrlen(images);
				arrput(images, specularMap);
				arrput(infos, specularInfo);
			}
			if (normalMap)
			{
				mat.ixMapNormal = arrlen(images);
				arrput(images, normalMap);
				arrput(infos, normalInfo);
			}
			if (alphaMap)
			{
				mat.ixMapAlpha = arrlen(images);
				arrput(images, alphaMap);
				arrput(infos, alphaInfo);
			}
			if (shininessMap)
			{
				mat.ixMapShininess = arrlen(images);
				arrput(images, shininessMap);
				arrput(infos, shininessInfo);
			}

			const uint32_t ixMaterial = mtlOffset + ixMtl;
			materials[ixMaterial] = mat;
			g.ixMaterial = ixMaterial;

		SKIP_MATERIAL:


			arrput(active->groups, g);
		}
		
		active->vertexCount  = (uint32_t)arrlen(obj.vertices);
		active->vertexOffset = vertexCount;
		active-> indexOffset = indexSize;

		vertexCount += (uint32_t)arrlen(obj.vertices);
		
		indexSize += indicesByteSize(active->vertexCount, arrlenu(obj.indices));

		//transfer ownership from obj to verticesV and indicesV
		arrput(verticesV, obj.vertices);
		arrput( indicesV, obj. indices);
		obj.vertices = NULL;
		obj. indices = NULL;

		//finalize mesh
		//TODO
	}

	//
	// Create Atlas: (prototype) 
	//

	//NOTE: the images will just be stacked in X direction.

	size_t
		atlasWidth = 0,
		atlasHeight = 0;
	const uint32_t ATLAS_PIXEL_ALIGNMENT = 128;

	const size_t mapCount = arrlenu(images);
	for (uint32_t i = 0; i < mapCount; i++)
	{
		struct TexInfo* mapInfo = infos + i;

		mapInfo->xOffset = (float)atlasWidth;
		mapInfo->yOffset = 0; //TODO

		atlasWidth += ceilTo(mapInfo->width, ATLAS_PIXEL_ALIGNMENT);
		atlasHeight = max(atlasHeight, ceilTo(mapInfo->height, ATLAS_PIXEL_ALIGNMENT));
	}



	//Now let's upload the vertices, indices, materials and images to the VBO, EBO, UBO & Atlas.

	const size_t
		vertexDataSize = vertexCount * sizeof(struct Vertex),
		  materialSize  = UBO_ALIGNMENT * arrlenu(materials);
	
	glNamedBufferData(vbo, vertexDataSize,	NULL, GL_STATIC_DRAW);
	glNamedBufferData(ebo,      indexSize,	NULL, GL_STATIC_DRAW);
	glNamedBufferData(ubo,   materialSize,	NULL, GL_STATIC_DRAW);
	glTextureStorage2D(atlas, 1, GL_RGBA8, atlasWidth, atlasHeight);



	uint8_t* pIndices = glMapNamedBuffer(ebo, GL_WRITE_ONLY);
	for (uint32_t ix = 0; ix < count; ix++)
	{
		struct mesh* m = meshes + ix;

		uint32_t indicesCount = (uint32_t)arrlen(indicesV[ix]);
		size_t indicesDataSize = indicesByteSize(m->vertexCount, indicesCount);

		size_t verticesDataSize = sizeof(struct Vertex) * m->vertexCount;
		size_t vertexByteOffset = sizeof(struct Vertex) * m->vertexOffset;

		glNamedBufferSubData(vbo, vertexByteOffset, verticesDataSize, verticesV[ix]);

		if (m->vertexCount > UINT16_MAX)
			memcpy_s(pIndices, indicesDataSize, indicesV[ix], indicesDataSize);
		else for (uint32_t i = 0; i < indicesCount; i++) 
			((uint16_t*)pIndices)[i] = (uint16_t)(indicesV[ix][i]);

		pIndices += indicesDataSize;
	}
	bool notCorrupted = glUnmapNamedBuffer(ebo);
	assert(notCorrupted);

	for (uint32_t i = 0; i < mapCount; i++)
	{
		struct TexInfo* info = infos + i;
		const stbi_uc* image = images[i];

		glTextureSubImage2D(atlas, 0, info->xOffset, info->yOffset, info->width, info->height, GL_RGBA, GL_UNSIGNED_BYTE, image);

		//change width and height into texCoord scale factor

		info->width		/= atlasWidth;
		info->height	/= atlasHeight;
		info->xOffset	/= atlasWidth;
		info->yOffset	/= atlasHeight;
	}

	const size_t materialCount = arrlenu(materials);
	uint8_t* pMaterial = glMapNamedBuffer(ubo, GL_WRITE_ONLY);
	for (uint32_t i = 0; i < materialCount; i++) 
	{
		const struct Material* m = materials + i;

		uploadToMaterialBuffer(pMaterial, m->ambient, m->diffuse, m->specular, m->alpha, m->shininess,
			m->ixMapAmbient == -1 ? NULL : infos + m->ixMapAmbient,
			m->ixMapDiffuse == -1 ? NULL : infos + m->ixMapDiffuse,
			m->ixMapSpecular== -1 ? NULL : infos + m->ixMapSpecular,
			m->ixMapNormal  == -1 ? NULL : infos + m->ixMapNormal,
			m->ixMapAlpha	== -1 ? NULL : infos + m->ixMapAlpha,
			m->ixMapShininess==-1 ? NULL : infos + m->ixMapShininess
		);

		pMaterial += UBO_ALIGNMENT;
	}
	notCorrupted = glUnmapNamedBuffer(ubo);
	assert(notCorrupted);

CLEANUP:
	for (uint32_t i = 0; i < arrlenu(images); i++)
	{
		stbi_image_free(images[i]);
	}
	for (uint32_t i = 0; i < arrlenu(verticesV); i++)
	{
		arrfree(verticesV[i]);
		arrfree( indicesV[i]);
	}
	arrfree(verticesV);
	arrfree( indicesV);
	arrfree(materials);
	arrfree(images);
	arrfree(infos);

	_aligned_free(mtllibPath);
	_aligned_free(imagePath);

	return meshes;

CLEANUP_INVALID: {
		cxERROR(_T("Failed to read object in \"%hs\"\n"), path);

		wfDestroyObj(&obj);
		wfDestroyMtllib(&mtllib);

		for (uint32_t i = 0; i < arrlen(meshes); i++)
			destroy_mesh(meshes + i);
		free(meshes);
		meshes = NULL;
		goto CLEANUP;
	}
}

static GLuint createUBO(GLsizei maxSize, GLenum usage)
{
	GLuint ubo;
	glCreateBuffers(1, &ubo);
	glNamedBufferData(ubo, maxSize, NULL, usage);
	return ubo;
}

#define COMPONENTS(vec) sizeof(vec)/sizeof(float)

static void GLAD_API_PTR GLDebugProc(
	_In_ GLenum source,
	_In_ GLenum type,
	_In_ GLuint id,
	_In_ GLenum severity,
	_In_ GLsizei length,
	_In_z_ const GLchar* message,
	_In_ const void* userParam)
{
	NkContext* ctx = (NkContext*)userParam;
	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR: cxERROR(_T("OpenGL: %hs\n"), message); break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: cxWARN(_T("OpenGL[Deprecated]: %hs\n"), message); break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: cxWARN(_T("OpenGL[Undefined]: %hs\n"), message); break;
	case GL_DEBUG_TYPE_PORTABILITY: cxWARN(_T("OpenGL[Portability]: %hs\n"), message); break;
	case GL_DEBUG_TYPE_PERFORMANCE: cxWARN(_T("OpenGL[Performance]: %hs\n"), message); break;
	default:
		cxINFO(_T("%hs\n"), message); break;
	}
}


/**
	@brief  Crox Application entry point
	@param  ctx  - Nuklear Context used with underlaying windowing mechanisms
	@param  argC - count of command line arguments
	@param  argV - vector of command line arguments
	@param  penv - environment varaibles
	@retval      - 
**/
extern int _tmain(_In_ NkContext* ctx, _In_ uint32_t argC, _In_ _TCHAR** argV, _In_z_ _TCHAR** penv)
{
	bool running = true;
#ifdef _DEBUG
	glDebugMessageCallback(GLDebugProc, ctx);
	glEnable(GL_DEBUG_OUTPUT);
#else
	glEnable(GL_NO_ERROR);
#endif // _DEBUG

	//
	// Driver Constants (specified by the driver, it's thus not possible to optimize around them)
	//

	const uint32_t 
		SSBO_ALIGNMENT = getDriverConstant(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT),
		 TBO_ALIGNMENT = getDriverConstant(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT),
		 UBO_ALIGNMENT = getDriverConstant(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT);

	const uint32_t 
		MAX_TEX_UNITS		= getDriverConstant(GL_MAX_TEXTURE_IMAGE_UNITS),
		MAX_UNIFORM_BINDINGS= getDriverConstant(GL_MAX_UNIFORM_BUFFER_BINDINGS);

	//
	// Setup Shaders
	//

	const GLuint program = makeProgramSPIRV(
		"3D_vert.spv", 
		NULL, 
		NULL, 
		NULL, 
		"3D_frag.spv");
	assert(program != 0);
	NAME_OBJECT(GL_PROGRAM, program, "<Blinn-Phong Shading Program>");

	glReleaseShaderCompiler();
	
	//
	// The Following Buffers pertain to the Blinn-Phong Shading Program:
	//

	GLuint
			     matrixUBO = createUBO(3 * sizeof(mat4), GL_DYNAMIC_DRAW),

			     cameraUBO = createUBO(1 * sizeof(vec4), GL_DYNAMIC_DRAW),

			   lightingUBO = createUBO(2 * sizeof(vec4), GL_DYNAMIC_DRAW),

		defaultMaterialUBO = createUBO(9 * sizeof(vec4),  GL_STATIC_DRAW);
	
	if			(matrixUBO)	NAME_OBJECT(GL_BUFFER,			matrixUBO,					 "<Matrix Uniform Buffer>");
	if			(cameraUBO)	NAME_OBJECT(GL_BUFFER,			cameraUBO,			"<Camera Position Uniform Buffer>");
	if		  (lightingUBO)	NAME_OBJECT(GL_BUFFER,		  lightingUBO,			"<Global Lighting Uniform Buffer>");
	if (defaultMaterialUBO) NAME_OBJECT(GL_BUFFER, defaultMaterialUBO, "<Defautlt Mesh's Material Uniform Buffer>");

	//
	//  Variables that in the future will be configurable (TODO: make configurable)
	//

	const vec3  DEFAULT_AMBIENT  = { 0.8f, 0.8f, 0.8f };
	const vec3  DEFAULT_DIFFUSE  = { 0.8f, 0.8f, 0.8f };
	const vec3  DEFAULT_SPECULAR = { 0.8f, 0.8f, 0.8f };
	const float DEFAULT_SHININESS= 1.45f;
	const float DEFAULT_ALPHA	 = 1.0f;

	const vec3 BEGIN_POS = { 0,0,-10 };		
	const float speed = .1f;				
	const float sensitivity = 1.f;		
	const float FOV = toRadians(45.0f);
	const float zNear = .1f;
	const float zFar = 100.0f;

	const vec3 lightPos = { 5, 5, 0 };
	const vec3 lightColor = { 1, 1, 1 };

	//
	// Specify Default Material
	//

	uint8_t* block = glMapNamedBuffer(defaultMaterialUBO, GL_WRITE_ONLY);
	{
		uploadToMaterialBuffer(block, DEFAULT_AMBIENT, DEFAULT_DIFFUSE, DEFAULT_SPECULAR, DEFAULT_ALPHA, DEFAULT_SHININESS, NULL, NULL, NULL, NULL, NULL, NULL);
	}
	const GLboolean notCorrupted = glUnmapNamedBuffer(defaultMaterialUBO);
	assert(notCorrupted); //TODO



	//
	// Create Meshes
	//

	GLuint vbo; // Vertices
	glCreateBuffers(1, &vbo);
	assert(vbo != 0);
	NAME_OBJECT(GL_BUFFER, vbo, "<Mesh Batch Vertices>");

	GLuint ebo; // Indices
	glCreateBuffers(1, &ebo);
	assert(ebo != 0);
	NAME_OBJECT(GL_BUFFER, ebo, "<Mesh Batch Indices>");

	GLuint mtls; // Materials
	glCreateBuffers(1, &mtls);
	assert(mtls != 0);
	NAME_OBJECT(GL_BUFFER, mtls,"<Mesh Batch Materials>");

	GLuint atlas;
	glCreateTextures(GL_TEXTURE_2D, 1, &atlas);
	assert(atlas != 0);
	NAME_OBJECT(GL_TEXTURE, atlas, "<Batch's Atlas>");

	//BIG TODO: fix this mess... and make it configurable too.
	const Path paths[] = {
		"../../../rsc/mesh/biplane/biplane.obj",
		"../../../rsc/mesh/cube.obj",
		"../../../rsc/mesh/globe.obj",
		"../../../rsc/mesh/Suzanne.obj",
		"../../../rsc/mesh/teapot.obj",
		"../../../rsc/mesh/sphere.obj"
	};
	const uint32_t meshCount = _countof(paths);
	struct mesh* meshes = createMeshes(meshCount, paths, vbo, ebo, mtls, atlas);
	assert(meshes != NULL);
	
	mat4 tmp;

	mat4x4_identity(meshes[0].matrix);
	
	//model = translation * rotation * scale 
	mat4x4_mul(meshes[1].matrix,
		mat4x4_translate(meshes[1].matrix, lightPos[0], lightPos[1], lightPos[2]),
		mat4x4_scale(tmp, mat4x4_identity(tmp), .1f));
	mat4x4_translate(meshes[2].matrix, 0, 5, 5);
	mat4x4_translate(meshes[3].matrix, 0, 0, 10);
	mat4x4_translate(meshes[4].matrix, 15, 0, 0);
	mat4x4_translate(meshes[5].matrix, 5, 5, 5);

	//
	// Specify Vertex Format
	//

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	assert(vao != 0);
	NAME_OBJECT(GL_VERTEX_ARRAY, vao, "<Batch Vertex Array Object>");

	const GLuint ixBind = 0;
	const size_t offset = 0L;
	const GLsizei stride = sizeof(struct Vertex);
	glVertexArrayVertexBuffer (vao, ixBind, vbo, offset, stride);
	glVertexArrayElementBuffer(vao, ebo);

	// layout(location = 0) in vec3 aXYZ
	glVertexArrayAttribBinding(vao, 0, ixBind);
	glVertexArrayAttribFormat (vao, 0, COMPONENTS(vec3), GL_FLOAT, false, offsetof(struct Vertex, pos));
	glEnableVertexArrayAttrib (vao, 0);

	// layout(location = 1) in vec3 aIJK
	glVertexArrayAttribBinding(vao, 1, ixBind);
	glVertexArrayAttribFormat (vao, 1, COMPONENTS(vec3), GL_FLOAT, false, offsetof(struct Vertex, normal));
	glEnableVertexArrayAttrib (vao, 1);

	// layout(location = 2) in vec2 aUV
	glVertexArrayAttribBinding(vao, 2, ixBind);
	glVertexArrayAttribFormat (vao, 2, COMPONENTS(vec2), GL_FLOAT, false, offsetof(struct Vertex, texcoord0));
	glEnableVertexArrayAttrib (vao, 2);

	for (uint32_t ix = 0; ix < meshCount; ix++)
	{
		struct mesh* m = &meshes[ix];
		m->program = program;
		m->vao = vao;
	}

	//
	// Specify sampling for Atlas
	//

	GLuint sampler;
	glCreateSamplers(1, &sampler);
	//GL_TEXTURE_MIN_LOD, GL_TEXTURE_MAX_LOD, GL_TEXTURE_LOD_BIAS GL_TEXTURE_COMPARE_MODE, or GL_TEXTURE_COMPARE_FUNC
	glSamplerParameteri (sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glSamplerParameteri (sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glSamplerParameteri (sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glSamplerParameteri (sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLfloat samplerBorderColor[4] = {0, 0, .2f, 1.0f};
	glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, samplerBorderColor);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, atlas);

	//
	// Render loop variables
	//

	uint32_t
		width = 0,
		height = 0;
	platform_getDimensions(ctx, &width, &height);
	assert(width != 0 && height != 0);

	struct Camera cam = { 0 };
	camera_projection_perspective(&cam,
		FOV,
		(float)width / height,
		zNear,
		zFar);
	camera_worldPosition_set(&cam, BEGIN_POS);
	camera_direction_set(&cam, NULL);

	bool hasRightClick = false;
	struct nk_vec2 rightClickOrgin;
	
	GLuint activeProgram = 0;
	GLuint activeVAO = 0;
	uint64_t lastRender=0;
	int returnstatus = 0;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0, 0, 0, 1.0f);
	glBindSampler(0, sampler);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	platform_show(ctx);
	while (running)
	{
#pragma region Input Handling
		nk_input_begin(ctx);
		running = platform_pollMessages(ctx, &returnstatus);
		if (!running) break; // Avoid swapping deleted buffers

		//
		// Handle Mouse and Key inputs
		//

		const struct nk_mouse* mouse = &ctx->input.mouse;
		const float scrollDelta = mouse->scroll_delta.y;
		const struct nk_input* in = &ctx->input;

		if (mouse->buttons[NK_BUTTON_RIGHT].down)
		{
			if (!hasRightClick)
			{
				hasRightClick = true;
				rightClickOrgin = mouse->pos;
			}
			else
			{
				float dX = mouse->pos.x - rightClickOrgin.x; //in mouse coordinates
				float dY = mouse->pos.y - rightClickOrgin.y; //in mouse coordinates

				float yaw  = toRadians( sensitivity * (dX ) / width);
				float pitch= toRadians( sensitivity * (dY ) / height);

				//
				// Note: input parameters say dX when dY is used to create the pitch constant; dX refers to the X pseudo-vector corresponding to the pitch axis, while the dY in this example refers to the moues movement
				//		

				camera_pitch_limited(&cam, -pitch, toRadians(85.f)); //go CCW
				camera_yaw(&cam, -yaw);
			}
		}
		else
		{
			hasRightClick = false;
		}

		
		vec3 direction = { 0,0,0 };
		if (scrollDelta)
		{
			vec3 tmp;
			camera_move_local(&cam, vec3_scale(tmp, cam.wDir, scrollDelta / 10));
		}
		for (int i = 0; i < in->keyboard.text_len; i++)
		{
			char c = in->keyboard.text[i];

			if (c == 'w')
				direction[2] += 1;
			else if (c == 's')
				direction[2] -= 1;

			else if (c == 'a')
				direction[0] += 1;
			else if (c == 'd')
				direction[0] -= 1;

			else if (c == 'e')
				direction[1] += 1;
			else if (c == 'q')
				direction[1] -= 1;
		}
		if (vec3_len(direction) != 0)
		{
			camera_move_local(&cam, vec3_scale(direction, direction, speed / vec3_len(direction)));
		}
		
		nk_input_end(ctx);
#pragma endregion
		
		//update viewport
		{
			uint32_t nwidth, nheight;
			platform_getDimensions(ctx, &nwidth, &nheight);
			if (nwidth != width || nheight != nheight)
			{
				width = nwidth;
				height = nheight;
				glViewport(0, 0, width, height);
				camera_projection_perspective(&cam, 
					FOV, 
					(float)width / height, 
					zNear, 
					zFar);
			}
		}
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		for (uint32_t ixMesh = 0; ixMesh < meshCount; ixMesh++)
		{
			const struct mesh* mesh = meshes + ixMesh;

			//
			// Setup Graphics Pipeline state
			//

			if (mesh->program != activeProgram)
			{
				glUseProgram(mesh->program);
				activeProgram = mesh->program;
			}
			if (mesh->vao != activeVAO)
			{
				activeVAO = mesh->vao;
				glBindVertexArray(mesh->vao);
			}

			//
			// Update Uniform buffers
			//

			mat4 model; //model matrix
			mat4x4_dup(model, mesh->matrix);

			mat4 normalMatrix; // = mat3(transpose(inverse(model)))
			mat4x4_transpose(normalMatrix, mat4x4_invert(normalMatrix, model));

			mat4 matrix; //projection * view * model 
			mat4x4_mul(matrix, cam.matrix, model); //matrix = matrix_cam * matrix_model			

			const GLint
				  ixMatrixBlock = glGetUniformBlockIndex(activeProgram,   "MatrixBlock"),
				  ixCameraBlock = glGetUniformBlockIndex(activeProgram,   "CameraBlock"),
				ixLightingBlock = glGetUniformBlockIndex(activeProgram, "LightingBlock"),
				ixMaterialBlock = glGetUniformBlockIndex(activeProgram, "MaterialBlock");
			
			if (ixMatrixBlock != -1)
			{
				uint8_t* block = (uint8_t*)glMapNamedBuffer(matrixUBO, GL_WRITE_ONLY);
				{
					memcpy(block + 0 * sizeof(mat4), matrix, sizeof matrix);
					
					memcpy(block + 1 * sizeof(mat4), model, sizeof model);
					
					memcpy(block + 2 * sizeof(mat4), normalMatrix, sizeof normalMatrix);
				}
				const GLboolean notCorrupted = glUnmapNamedBuffer(matrixUBO);
				
				if (!notCorrupted)
				{
					//deal with corruption
					GLint size;
					glGetNamedBufferParameteriv(matrixUBO, GL_BUFFER_SIZE, &size);
					glDeleteBuffers(1, &matrixUBO);
					matrixUBO = createUBO(size, GL_DYNAMIC_DRAW);
					continue; //give a chance to set the values again in the new buffer
				}

				glBindBufferBase(GL_UNIFORM_BUFFER, 0, matrixUBO);
			}
			if (ixCameraBlock != -1)
			{			
				uint8_t* block = (uint8_t*)glMapNamedBuffer(cameraUBO, GL_WRITE_ONLY);
				{
					memcpy(block, cam.wPos, sizeof cam.wPos);
				}
				const GLboolean notCorrupted = glUnmapNamedBuffer(cameraUBO);
				assert(notCorrupted); //TODO

				glBindBufferBase(GL_UNIFORM_BUFFER, 1, cameraUBO);
			}
			if (ixLightingBlock != -1)
			{ 			
				uint8_t* block = (uint8_t*)glMapNamedBuffer(lightingUBO, GL_WRITE_ONLY);
				{
					//u_lightPos
					memcpy(block + 0 * sizeof(vec4), lightPos, sizeof lightPos);
					//u_lightColor
					memcpy(block + 1 * sizeof(vec4), lightColor, sizeof lightColor);
				}
				const GLboolean notCorrupted = glUnmapNamedBuffer(lightingUBO);
				assert(notCorrupted); //TODO
				
				glBindBufferBase(GL_UNIFORM_BUFFER, 2, lightingUBO);
			}

			//
			// Setup the material for each group and draw.
			//
			
			const GLenum indexStoreType = mesh->vertexCount > UINT16_MAX ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
			const size_t groupCount = arrlen(mesh->groups);
			if (groupCount == 0)
			{
				glBindBufferBase(GL_UNIFORM_BUFFER, 3, defaultMaterialUBO);
				glDrawArrays(GL_TRIANGLES, mesh->vertexOffset, mesh->vertexCount);
			}
			else for (uint32_t ixG = 0; ixG < groupCount; ixG++)
			{
				const struct group* g = &mesh->groups[ixG];

				if (ixMaterialBlock != -1)
				{
					if (g->ixMaterial != -1)
						glBindBufferRange(GL_UNIFORM_BUFFER, 3, mtls, g->ixMaterial * UBO_ALIGNMENT, 0x50);
					else
						glBindBufferBase(GL_UNIFORM_BUFFER, 3, defaultMaterialUBO);
				}
				
				//
				// When Meshes are batched, the meshes are stored sequencialy in both buffers.
				//

				if (mesh->vertexOffset != 0)
					glDrawElementsBaseVertex(GL_TRIANGLES, g->count, indexStoreType, (void*)((indexStoreType == GL_UNSIGNED_INT ? sizeof(int) : sizeof(short)) *g->ixFirst + mesh->indexOffset), mesh->vertexOffset);
				else
					glDrawElements(GL_TRIANGLES, g->count, indexStoreType, (void*)((indexStoreType == GL_UNSIGNED_INT ? sizeof(int) : sizeof(short)) * g->ixFirst + mesh->indexOffset));
			}
		}

		const bool swapSuccess = platform_swapBuffers(ctx);
		assert(swapSuccess);
	}
	
	//
	// Cleanup
	//

	for (uint32_t i = 0; i < meshCount; i++) 
		destroy_mesh(meshes + i);
	free(meshes);

	glDeleteSamplers(1, &sampler);
	glDeleteTextures(1, &atlas);
	const GLuint buffersToDelete[] = { vbo, ebo, mtls, 
		matrixUBO, cameraUBO, lightingUBO, defaultMaterialUBO };
	glDeleteBuffers(_countof(buffersToDelete), buffersToDelete);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

	return returnstatus;
}
