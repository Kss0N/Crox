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

#include <glad/gl.h>
#include <glad/wgl.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <linmath.h>
#include <cwalk.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <stb_include.h>
#include <tchar.h>





#ifdef _DEBUG
int OutputDebugFormatted(
	_In_z_ _Printf_format_string_params_(1) const TCHAR* format, ...)
{
	TCHAR buffer[4096];

	va_list args;
	va_start(args, format);

	int result = _vstprintf(buffer, 1024, format, args);
	if (result != 0)
	{
		OutputDebugString(buffer);
	}
	va_end(args);
	return result;
}
#else
#define OutputDebugFormatted(f, ...)
#endif // _DEBUG



float toRadians(float deg)
{
	return deg * (float)(180.0 / M_PI);
}


#define NAME_OBJECT(type, obj, name) glObjectLabel(type, obj, -(signed)strlen(name),name);

typedef _Null_terminated_ const char* Path;

GLuint makeShader(GLenum type, const char* path)
{
	char error[256];
	char* source = stb_include_file(path, NULL, NULL, error);
	if (source == NULL)
	{
		glDebugMessageInsert(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_HIGH, -(signed)strlen(error), error); 
		return 0;
	}
	size_t length = strlen(source);

	GLuint shader = glCreateShader(type);
	NAME_OBJECT(GL_SHADER, shader, path);

	glShaderSource(shader, 1, &source, &length);
	glCompileShader(shader);

	GLint isCompiled = false;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);

#ifndef glDebugMessageCallback
	if (!isCompiled)
	{
		GLuint len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
		assert(len != 0);
		const char* msg = malloc(len * sizeof * msg);
		glGetShaderInfoLog(shader, len, NULL, msg);
		OutputDebugStringA(msg);
		free(msg);

		glDeleteShader(shader);
		shader = 0;
	}
#endif // glDebugMessageCallback
	
	if (!isCompiled)
	{
		glDeleteShader(shader);
		shader = 0;
	}

	free(source);
	return shader;
}

GLuint makeProgramGLSL(
	_In_		Path		vertex,
	_In_opt_	Path		tessEval,
	_In_opt_	Path		tessCtrl,
	_In_opt_	Path		geometry,
	_In_		Path		fragment)
{
	return 0;
}

const unsigned char* readBinary(_In_z_ Path p, _Out_ size_t* len)
{
	FILE* f;
	fopen_s(&f, p, "rb");
	if (!f)
	{
		OutputDebugFormatted(_T("Could not open path <%hs>\n"), p);
		*len = 0;
		return NULL;
	}

	fseek(f, 0, SEEK_END);

	*len = (size_t) ftell(f);
	fseek(f, 0, SEEK_SET);
	char* data = malloc(*len+1);
	assert(data != NULL); //TODO

	
	size_t readCount = fread_s(data, *len, 1, *len, f);
	assert(readCount == *len);

	fclose(f);
	return data;
}

inline void cleanupSource(const char* src)
{
	if (src != NULL)
		free(src);
}

inline void detachDeleteShader(GLuint fromProgram, GLuint shader)
{
	if (shader) 
	{
		glDetachShader(fromProgram, shader);
		glDeleteShader(shader);
	}
}

#ifdef _DEBUG
inline bool checkCompileStatus(GLuint shader) 
{
	GLint isCompiled = GL_FALSE;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
	if (!isCompiled)
	{
		GLint msgLen;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &msgLen);
		
		GLchar* msg = alloca(msgLen+1);
		glGetShaderInfoLog(shader, msgLen, NULL, msg);

		OutputDebugFormatted(_T("Error compiling shader: %hs"), msg);
	}
	return isCompiled;
}
#else
#define checkCompileStatus(shader)
#endif // _DEBUG

inline bool isSPIRV(GLuint shader)
{
	GLint isSpirv = false;
	glGetShaderiv(shader, GL_SPIR_V_BINARY, &isSpirv);
	return isSpirv;
}

static void compileAndAttachSPIRV(_In_ GLuint shader, _In_ GLuint program, _In_ GLsizei codeLength, _In_reads_bytes_(codeLength) GLchar* code)
{
	glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, code, codeLength);
	assert(isSPIRV(shader));
	glSpecializeShader(shader, "main", 0, NULL, NULL);
	assert(checkCompileStatus(shader));
	glAttachShader(program, shader);
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
_Must_inspect_result_ _Success_(return != 0) GLuint 
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
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 2, GL_DEBUG_SEVERITY_HIGH, -1, "The Vendor does not Support SPIRV");
		return 0;
	}
	assert(spirvSupport != GL_FALSE);

	if (tessEval && !tessCtrl)
		OutputDebugString(TEXT("WARING: creating program with tesselation evaluation without control.\n"));
#endif // _DEBUG	

	GLuint program = 0;

	size_t 
		 vSrcLen = 0,
		tcSrcLen = 0,
		teSrcLen = 0,
		 gSrcLen = 0,
		 fSrcLen = 0;

	const uint8_t*
		 vSrc = readBinary( vertex, &vSrcLen),						//	   Vertex SPIRV Source
		teSrc = tessEval ? readBinary(tessEval, &tcSrcLen) : NULL,	//	Tess Eval SPIRV source
		tcSrc = tessCtrl ? readBinary(tessCtrl, &teSrcLen) : NULL,	//	Tess Ctrl SPIRV source
		 gSrc = geometry ? readBinary(geometry, & gSrcLen) : NULL,	//   Geometry SPIRC Source
		 fSrc = readBinary(fragment, &fSrcLen);						//   fragment SPIRV Source
	
	if (!vSrc				||
		(tessEval && !tcSrc)||
		(tessCtrl && !teSrc)||
		(geometry && ! gSrc)||
		!fSrc)
	{
		OutputDebugFormatted(_T("ERROR: could not read one or more SPIR-V files"));
		goto CLEANUP_SOURCE;
	}

	GLuint
		 vShader = glCreateShader(GL_VERTEX_SHADER),						// Vertex shader
		teShader = tessEval ? glCreateShader(GL_TESS_EVALUATION_SHADER) : 0,// Tesselation evalutation shader (optional)
		tcShader = tessCtrl ? glCreateShader(GL_TESS_CONTROL_SHADER)	: 0,// Tesselation control shader (optional)
		 gShader = geometry ? glCreateShader(GL_GEOMETRY_SHADER)		: 0,// Geometry shader (optional)
		 fShader = glCreateShader(GL_FRAGMENT_SHADER);						// Fragment shader		

	NAME_OBJECT(GL_SHADER, vShader, vertex);
	if (tessEval) NAME_OBJECT(GL_SHADER, teShader, tessEval);
	if (tessCtrl) NAME_OBJECT(GL_SHADER, tcShader, tessCtrl);
	if (geometry) NAME_OBJECT(GL_SHADER,  gShader, geometry);
	NAME_OBJECT(GL_SHADER, fShader, fragment);

	program = glCreateProgram();

	compileAndAttachSPIRV(vShader, program, vSrcLen, vSrc);
	if (teShader) compileAndAttachSPIRV(teShader, program, teSrcLen, teSrc);
	if (tcShader) compileAndAttachSPIRV(tcShader, program, tcSrcLen, tcSrc);
	if ( gShader) compileAndAttachSPIRV( gShader, program,  gSrcLen,  gSrc);
	compileAndAttachSPIRV(fShader, program, fSrcLen, fSrc);
	
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


struct group {
	uint32_t ixFirst;
	uint32_t count;

	bool hasMaterial;
	struct WavefrontMtl mtl;
};

struct mesh
{
	struct group* groups;	//stb_ds darray
	LPCSTR name;


	uint32_t vertexCount;	//count of elements
	uint32_t vertexOffset;	//count of preceeding elements
	uint32_t indexOffset;	//count of preceeding elements

	GLuint vao;
	GLuint program;

	mat4 matrix;
	
};

void destroy_mesh(struct mesh* m)
{
	arrfree(m->groups);
	
	if (m->name) 
	{
		_aligned_free(m->name);
		m->name = NULL;
	}
	m->vertexCount = 0;
	m->vertexOffset = 0;
}






_Check_return_ _Success_(return != NULL)
/**
	@brief  Creates a stb_ds darray of meshes read from the paths. All vertex data and index data will be stored sequencialy in the vbo and ebo supplied as parameter.
	@param  paths - 
	@param  vbo   - 
	@param  ebo   - 
	@retval       - 
**/
inline struct mesh* createMeshes(_In_ Path* paths, _Inout_ const GLuint vbo, _Inout_ const GLuint ebo)
{
	struct mesh* meshes = NULL;
	
	uint32_t vertexCount = 0;
	uint32_t  indexCount = 0;
	struct Vertex** verticesV = NULL;	// stb_ds Vector of vertex arrays
	uint32_t** indicesV = NULL;			// stb_ds Vector of element arrays

	struct WavefrontObj obj;
	struct WavefrontMtllib mtllib;

	char* mtllibPath = _aligned_malloc(MAX_PATH, 8);

	struct mesh active;
	Path p;

	uint32_t count = (uint32_t)arrlen(paths);
	for (uint32_t ix = 0;
		ix < count;
		++ix, wfDestroyObj(&obj), wfDestroyMtllib(&mtllib))
	{
		p = paths[ix];
		FILE* file;
		errno_t err;  

		active.groups = NULL;
		active.name = NULL;
		
		err = fopen_s(&file, p, "r");
		if (err != NULL)
			goto CLEANUP_INVALID;
		obj = wavefront_obj_read(file);
		assert(obj.vertices != NULL);
		fclose(file);

		if (obj.mtllib != NULL)
		{
			cwk_path_change_basename(p, obj.mtllib, mtllibPath, MAX_PATH);
			err = fopen_s(&file, mtllibPath, "r");
			if (err != 0)
				goto CLEANUP_INVALID;
			mtllib = wavefront_mtl_read(file);
			assert(mtllib.keys != NULL);
			fclose(file);
		}
		

		if (obj.name)
		{
			//Let's be a bit clever and just transfer ownership.
			active.name = obj.name;
			obj.name = NULL;

		}
		
		OutputDebugFormatted(_T("Creating mesh \"%hs\".\n"), active.name ? active.name : "NONAME");

		uint32_t groupCount = (uint32_t)arrlen(obj.groups);
		for (uint32_t ixG = 0; ixG < groupCount; ixG++)
		{
			struct WavefrontGroup* group = obj.groups + ixG;
			struct group g = {
				.ixFirst = group->ixFirst,
				.count = group->count,
				.hasMaterial = false,
			};

			if (obj.mtllib != NULL)
			{
				g.hasMaterial = true;
				g.mtl = shget(mtllib.materialMap, group->mtlName);
			}
			arrput(active.groups, g);
		}
		active.vertexCount = arrlen(obj.vertices);
		active.vertexOffset = vertexCount;
		active.indexOffset = indexCount;

		vertexCount += arrlen(obj.vertices);
		indexCount += arrlen(obj.indices);

		//transfer ownership from obj to verticesV and indicesV
		arrput(verticesV, obj.vertices);
		arrput( indicesV, obj. indices);
		obj.vertices = NULL;
		obj. indices = NULL;

		
		

		//finalize and append mesh
		arrput(meshes, active);
	}

	//Now let's upload the vertices and indices to the VBO and EBO

	size_t vertexDataSize = sizeof(struct Vertex) * vertexCount;
	size_t  indexDataSize = sizeof(uint32_t) * indexCount;

	glNamedBufferData(vbo, vertexDataSize, NULL, GL_STATIC_DRAW);
	glNamedBufferData(ebo, indexDataSize, NULL, GL_STATIC_DRAW);

	uint32_t indexOffset = 0;
	uint32_t vertexOffset = 0;
	for (uint32_t ix = 0; ix < count; ix++)
	{
		struct mesh* m = meshes + ix;

		uint32_t indicesCount = (uint32_t)arrlen(indicesV[ix]);

		size_t indicesDataSize = sizeof(uint32_t) * indicesCount;
		size_t indexByteOffset = sizeof(uint32_t) * indexOffset;

		size_t verticesDataSize = sizeof(struct Vertex) * m->vertexCount;
		size_t vertexByteOffset = sizeof(struct Vertex) * m->vertexOffset;

		glNamedBufferSubData(vbo, vertexByteOffset, verticesDataSize, verticesV[ix]);
		glNamedBufferSubData(ebo,  indexByteOffset,  indicesDataSize,  indicesV[ix]);
		
		assert(vertexOffset == m->vertexOffset);

		 indexOffset += arrlen( indicesV[ix]);
		vertexOffset += arrlen(verticesV[ix]);
		
	}
	assert(indexOffset == indexCount);
	assert(vertexOffset == vertexCount);

CLEANUP:
	for (uint32_t i = 0; i < arrlen(verticesV); i++)
	{
		arrfree(verticesV[i]);
		arrfree( indicesV[i]);
	}
	arrfree(verticesV);
	arrfree(indicesV);
	_aligned_free(mtllibPath);
	return meshes;

CLEANUP_INVALID:

	OutputDebugFormatted(_T("Failed to read object in \"%hs\""), p);

	wfDestroyObj(&obj);
	wfDestroyMtllib(&mtllib);
	destroy_mesh(&active);

	for (uint32_t i = 0; i < arrlen(meshes); i++)
		destroy_mesh(meshes + i);
	arrfree(meshes);
	goto CLEANUP;
}



void GLAD_API_PTR GLDebugProc(
	_In_ GLenum source,
	_In_ GLenum type,
	_In_ GLuint id,
	_In_ GLenum severity,
	_In_ GLsizei length,
	_In_ const GLchar* message,
	_In_ const void* userParam)
{
	NkContext* ctx = (NkContext*)userParam;

	OutputDebugStringA(message);
	OutputDebugString(TEXT("\n"));
}


/**
	@brief  Crox Application entry point
	@param  ctx  - 
	@param  argC - 
	@param  argV - 
	@param  penv - 
	@retval      - 
**/
int _tmain(_In_ NkContext* ctx, _In_ uint32_t argC, _In_ _TCHAR** argV, _In_ _TCHAR** penv)
{
	bool running = true;
#ifdef _DEBUG
	glDebugMessageCallback(GLDebugProc, ctx);
	glEnable(GL_DEBUG_OUTPUT);
#else
	glEnable(GL_NO_ERROR);
#endif // _DEBUG

	uint32_t width = 0;
	uint32_t height = 0;
	platform_getDimensions(ctx, &width, &height);
	assert(width != 0 && height != 0);
	glViewport(0, 0, width, height);

	/*
#ifdef _DEBUG
	GLint extCount = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &extCount);
	for (GLint i = 0; i < extCount; i++) OutputDebugFormatted(_T("\t%hs\n"), (const char*)glGetStringi(GL_EXTENSIONS, i));
	OutputDebugFormatted(_T("%hs\n"), wglGetExtensionsStringEXT());
#endif // _DEBUG
*/


	GLuint program = makeProgramSPIRV(
		"3D_vert.spv", 
		NULL, 
		NULL, 
		NULL, 
		"3D_frag.spv");
	assert(program != 0);
	NAME_OBJECT(GL_PROGRAM, program, "<Basic 3D Program>");

	glReleaseShaderCompiler();
	
	/*
	//setup program using <3D.vert> + <3D.frag>
	{


		GLuint
			vShader = makeShader(GL_VERTEX_SHADER, "3D_vert.spv"),
			fShader = makeShader(GL_FRAGMENT_SHADER, "3D.frag");
		assert(vShader != 0);
		assert(fShader != 0);

		glAttachShader(program, vShader);
		glAttachShader(program, fShader);

		glLinkProgram(program);

		GLuint isLinked = false;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
		assert(isLinked);

		glDetachShader(program, vShader);
		glDetachShader(program, fShader);
		glDeleteShader(vShader);
		glDeleteShader(fShader);
	}
	*/

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	assert(vao != 0);
	NAME_OBJECT(GL_VERTEX_ARRAY, vao, "<Mesh Vertex Array>");

	GLuint vbo;
	glCreateBuffers(1, &vbo);
	assert(vbo != 0);
	NAME_OBJECT(GL_BUFFER, vbo, "<Mesh Vertices>");

	GLuint ebo;
	glCreateBuffers(1, &ebo);
	assert(ebo != 0);
	NAME_OBJECT(GL_BUFFER, ebo, "<Mesh Indices>");



	Path* paths = NULL;
	arrput(paths, "../rsc/mesh/biplane/biplane.obj");
	arrput(paths, "../rsc/mesh/cube.obj");
	arrput(paths, "../rsc/mesh/globe.obj");
	arrput(paths, "../rsc/mesh/Suzanne.obj");
	arrput(paths, "../rsc/mesh/teapot.obj");
	arrput(paths, "../rsc/mesh/sphere.obj");

	struct mesh* meshes = createMeshes(paths, vbo, ebo);
	arrfree(paths);

	assert(meshes != NULL);

	for (uint32_t ix = 0; ix < arrlen(meshes); ix++)
	{
		struct mesh* m = &meshes[ix];
		m->program = program;
		m->vao = vao;
	}

	GLuint ixBind = 0;
	size_t offset = 0L;
	size_t stride = sizeof(struct Vertex);
	glVertexArrayVertexBuffer (vao, ixBind, vbo, offset, stride);
	glVertexArrayElementBuffer(vao, ebo);

	// layout(location = 0) in vec3 aXYZ
	glVertexArrayAttribBinding(vao, 0, ixBind);
	glVertexArrayAttribFormat (vao, 0, 3, GL_FLOAT, false, offsetof(struct Vertex, pos));
	glEnableVertexArrayAttrib (vao, 0);

	// layout(location = 1) in vec3 aIJK
	glVertexArrayAttribBinding(vao, 1, ixBind);
	glVertexArrayAttribFormat (vao, 1, 3, GL_FLOAT, false, offsetof(struct Vertex, normal));
	glEnableVertexArrayAttrib (vao, 1);

	// layout(location = 2) in vec2 aUV
	glVertexArrayAttribBinding(vao, 2, ixBind);
	glVertexArrayAttribFormat (vao, 2, 2, GL_FLOAT, false, offsetof(struct Vertex, texcoord0));
	glEnableVertexArrayAttrib (vao, 2);

	vec3 lightPos = { 5, 5, 0 };
	vec3 lightColor = { 1, 1, 1 };

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

	vec3 pos = {0,0,-10};
	vec3 dir = {0,0,1};
	const vec3 UP = { 0,1,0 };
	float speed = .1f;

	struct Camera cam = { 0 };
	camera_projection_perspective(&cam, M_PI / 4, (float)width / height, .1, 100);
	camera_view_set(&cam, pos, dir);

	bool hasRightClick = false;
	struct nk_vec2 rightClickOrgin;
	GLuint activeProgram = 0;
	GLuint activeVAO = 0;
	int result = 0;

	uint64_t lastRender;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glClearColor(0, 0, 0, 1.0f);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	platform_show(ctx);
	while (running)
	{
#pragma region Input Handling
		nk_input_begin(ctx);
		running = platform_pollMessages(ctx, &result);
		if (!running) break; // Avoid swapping deleted buffers


		struct nk_mouse mouse = ctx->input.mouse;

		if (mouse.buttons[NK_BUTTON_RIGHT].down)
		{
			if (!hasRightClick)
			{
				hasRightClick = true;
				rightClickOrgin = mouse.pos;
			}
			else
			{
				float dX = mouse.pos.x - rightClickOrgin.x;
				float dY = mouse.pos.y - rightClickOrgin.y;

				float yaw  = toRadians( 0.001 * (dX ) / width);
				float pitch= toRadians( 0.001 * (dY ) / height);
				
				//vertically:
				//newDir = rotate(dir, -yaw, |dir x up|) = rotate(-yaw * |dir x up|) * dir

				vec3 right; 
				mat4 tmpM;

				vec3_norm(right, vec3_mul_cross(right, dir, UP));
				mat4x4_rotate(tmpM, mat4x4_identity(tmpM), right[0], right[1], right[2], -pitch);

				vec4 newDir;
				mat4x4_mul_vec4(newDir, tmpM, dir);

				float ang = vec3_angle(newDir, UP);
				float dAng = vec3_angle(newDir, dir);

				//don't allow upside down rotation
				if (fabs(vec3_angle(newDir, UP)) - toRadians(90) <= toRadians(85))
					vec3_dup(dir, newDir);

				// horizontally:
				// dir = rotate(dir, -yaw, Up)

				vec4 newDir2;
				mat4x4_mul_vec4(newDir2, mat4x4_rotate(tmpM, mat4x4_identity(tmpM), UP[0], UP[1], UP[2], -yaw), newDir);
				vec3_dup(dir, newDir2);
			}
		}
		else
		{
			hasRightClick = false;
		}

		float scrollDelta = mouse.scroll_delta.y;
		if (scrollDelta)
		{
			vec3 tmp;
			vec3_add(dir, dir, vec3_scale(tmp, dir, scrollDelta / 10));

		}

		struct nk_input* in = &ctx->input;
		for (int i = 0; i < in->keyboard.text_len; i++)
		{
			char c = in->keyboard.text[i];

			vec3 temp;

			if (c == 'w')
				vec3_add(pos, pos, vec3_scale(temp, dir, speed));
			else if (c == 's')
				vec3_add(pos, pos, vec3_scale(temp, dir, -speed));
			else if (c == 'a')
				vec3_add(pos, pos, vec3_scale(temp, vec3_norm(temp, vec3_mul_cross(temp, UP, dir)), speed));
			else if (c == 'd')
				vec3_add(pos, pos, vec3_scale(temp, vec3_norm(temp, vec3_mul_cross(temp, UP, dir)), -speed));
			else if (c == 'e')
				vec3_add(pos, pos, vec3_scale(temp, UP, speed));
			else if (c == 'q')
				vec3_add(pos, pos, vec3_scale(temp, UP, -speed));

			
		}
		camera_view_set(&cam, pos, dir);

		
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
				camera_projection_perspective(&cam, M_PI / 4, (float)width / height, .1, 100);
			}
		}
		

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		
		uint32_t meshCount = arrlen(meshes);
		for (uint32_t ixM = 0; ixM < meshCount; ixM++)
		{
			struct mesh* mesh = meshes + ixM;

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

			GLint
				camPosLocation = glGetUniformLocation(activeProgram, "u_camPos"),
				lightPosLocation = glGetUniformLocation(activeProgram, "u_lightPos"),
				lightColorLocation = glGetUniformLocation(activeProgram, "u_lightColor");

			if (camPosLocation != -1)
				glProgramUniform3fv(activeProgram, camPosLocation, 1, pos);
			if (lightPosLocation != -1)
				glProgramUniform3fv(activeProgram, lightPosLocation, 1, lightPos);
			if (lightColorLocation != -1)
				glProgramUniform3fv(activeProgram, lightColorLocation, 1, lightColor);


			mat4 model;
			mat4x4_dup(model, mesh->matrix);

			mat4 matrix;//model matrix
			mat4x4_mul(matrix, cam.matrix, model); //matrix = cam's matrix * E

			mat4 normalMatrix; // = mat3(transpose(inverse(model)))
			mat4x4_transpose(normalMatrix, mat4x4_invert(normalMatrix, model));

			GLint
				modelMatrixLocation = glGetUniformLocation(activeProgram, "u_model"),
				projectionViewModelMatrixLocation = glGetUniformLocation(activeProgram, "u_matrix"),
				normalMatrixLocation = glGetUniformLocation(activeProgram, "u_normalMatrix");

			if (modelMatrixLocation != -1)
				glProgramUniformMatrix4fv(activeProgram, modelMatrixLocation, 1, false, model);
			if (projectionViewModelMatrixLocation != -1)
				glProgramUniformMatrix4fv(activeProgram, projectionViewModelMatrixLocation, 1, false, matrix);
			if (normalMatrixLocation != -1)
				glProgramUniformMatrix4fv(activeProgram, normalMatrixLocation, 1, false, normalMatrix);
			

			uint32_t groupCount = arrlen(mesh->groups);
			if (groupCount == 0)
			{
				glDrawArrays(GL_TRIANGLES, mesh->vertexOffset, mesh->vertexCount);
			}
			else for (uint32_t ixG = 0; ixG < groupCount; ixG++)
			{
				struct group g = mesh->groups[ixG];

				if (g.hasMaterial)
				{
					GLint
						ambientColorLocation = glGetUniformLocation(activeProgram, "u_ambientColor"),
						diffuseColorLocation = glGetUniformLocation(activeProgram, "u_diffuseColor"),
						specularColorLocation= glGetUniformLocation(activeProgram, "u_specularColor"),
						alphaLocation		 = glGetUniformLocation(activeProgram, "u_alpha"),
						shininessLocation	 = glGetUniformLocation(activeProgram, "u_shininess");

					if (ambientColorLocation != -1)
						glProgramUniform3fv(activeProgram, ambientColorLocation, 1, g.mtl.ambient);
					if (diffuseColorLocation != -1) 
						glProgramUniform3fv(activeProgram, diffuseColorLocation, 1, g.mtl.diffuse);
					if (specularColorLocation != -1)
						glProgramUniform3fv(activeProgram, specularColorLocation,1, g.mtl.specular);
					if(alphaLocation != -1)
						glProgramUniform1f(activeProgram, alphaLocation, g.mtl.alpha);
					if(shininessLocation != -1)
						glProgramUniform1f(activeProgram, shininessLocation, g.mtl.shininess);
				}

				if (mesh->vertexOffset != 0)
					//
					// When there are multiple meshes stored in the same vbo and ebo,
					// the meshes are stored sequencialy in both buffers.
					//
					glDrawElementsBaseVertex(GL_TRIANGLES, g.count, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * (g.ixFirst + mesh->indexOffset)), mesh->vertexOffset);
				else
					glDrawElements(GL_TRIANGLES, g.count, GL_UNSIGNED_INT, (void*)(sizeof(uint32_t) * (g.ixFirst + mesh->indexOffset)));
			}
		}
		

		BOOL swapSuccess = platform_swapBuffers(ctx);
		assert(swapSuccess);
	}



	uint32_t meshCount = arrlen(meshes);
	for (uint32_t i = 0; i < meshCount; i++) destroy_mesh(meshes + i);
	arrfree(meshes);

	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

	return result;
}
