#include "Crox.h"
#include "platform/Platform.h"
#include "framework_crt.h"
#include "framework_spirv.h"
#include "mesh.h"
#include "wavefront.h"

#include <glad/gl.h>
#include <glad/wgl.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <linmath.h>
#include <cwalk.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <stb_include.h>


typedef mat4x4 mat4;

struct Camera 
{
//plz don't modify directly
	mat4 matrix;
	mat4 _view;
	mat4 _projection;
};
#define UPDATE_CAMERA_MATRIX(cam) mat4x4_mul(cam->matrix, cam->_projection, cam->_view);
void setProjection(struct Camera* cam, mat4 projection) 
{
	memcpy_s(cam->_projection, sizeof(mat4), projection, sizeof(mat4));

	UPDATE_CAMERA_MATRIX(cam);
}

void setView(struct Camera* cam, vec3 pos, vec3 dir)
{

	vec3 center;
	
	const vec3 up = { 0, 1.0f, 0 };

	mat4x4_look_at(cam->_view, pos, vec3_add(center, pos, dir), up);

	UPDATE_CAMERA_MATRIX(cam);
}




#define NAME_OBJECT(type, obj, name) glObjectLabel(type, obj, -(signed)strlen(name),name);

typedef _In_z_ const char* Path;

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
	
}




inline void cleanupSource(const char* src)
{
	if (src != 0)
		free(src);
}

inline void detachDeleteShader(GLuint fromProgram, GLuint shader)
{
	if (shader) {
		glDetachShader(fromProgram, shader);
		glDeleteShader(shader);
	}
}

/**
	@brief  Creates a OpenGL program using SPIRV sources. Sources must have "main" as entry point.
	@param  vertex   - path to vertex shader SPIRV 
	@param  tessEval - path to tesselation evaluation shader SPIRV. If the tessEval stage is skipped, this parameter is NULL, though SHOULD be followed by tessEval.
	@param  tessCtrl - path to tesselation control shader SPIRV. If the tessCtrl stage is skipped, this parameter is NULL, though SHOULD be followed by tessCtrl.
	@param  geometry - path to geometry shader SPIRV. If the geometry stage is skipped, this parameter is NULL
	@param  fragment - path to fragment shader SPIRV.
	@retval          - valid program. 0 on failure
**/
_Must_inspect_result_ _Success_(return != 0) GLuint 
makeProgramSPIRV(
	_In_		Path		  vertex,
	_In_opt_	Path		tessEval,
	_In_opt_	Path		tessCtrl,
	_In_opt_	Path		geometry,
	_In_		Path		fragment)
{
	GLint spirvSupport = GL_FALSE;
	glGetIntegerv(GL_NUM_SHADER_BINARY_FORMATS, &spirvSupport);
	assert(spirvSupport != GL_FALSE);
	if (!spirvSupport)
	{
		glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_ERROR, 2, GL_DEBUG_SEVERITY_HIGH, -1, "The Vendor does not Support SPIRV");
		return 0;
	}

	assert(vertex != NULL); 
	assert(fragment != NULL);
	
	if (tessEval && !tessCtrl)
		OutputDebugString(TEXT("WARING: creating program with tesselation evaluation without control.\n"));

	const err[256];
	memset(err, 0, sizeof err);

	const char*
		 vSrc = stb_include_file(vertex, NULL, NULL, err),
		tcSrc = tessEval ? stb_include_file(tessEval, NULL, NULL, err) : 0,
		teSrc = tessCtrl ? stb_include_file(tessCtrl, NULL, NULL, err) : 0,
		 gSrc = geometry ? stb_include_file(geometry, NULL, NULL, err) : 0,
		 fSrc = stb_include_file(fragment, NULL, NULL, err);
	
	if (!vSrc				||
		(tessEval && !tcSrc)||
		(tessCtrl && !teSrc)||
		(geometry && !gSrc) ||
		!fSrc)
	{
		glDebugMessageInsert(GL_DEBUG_SOURCE_THIRD_PARTY, GL_DEBUG_TYPE_ERROR, 1, GL_DEBUG_SEVERITY_HIGH, -(signed)strlen(err), err);

		cleanupSource(vSrc);
		cleanupSource(teSrc);
		cleanupSource(tcSrc);
		cleanupSource(gSrc);
		cleanupSource(fSrc);

		return 0;
	}

	GLuint
		vShader = glCreateShader(GL_VERTEX_SHADER),	// Vertex shader
		teShader = tessEval ? glCreateShader(GL_TESS_EVALUATION_SHADER) : 0,// Tesselation evalutation shader (optional)
		tcShader = tessCtrl ? glCreateShader(GL_TESS_CONTROL_SHADER)	: 0,	// Tesselation control shader (optional)
		gShader = geometry ? glCreateShader(GL_GEOMETRY_SHADER)			: 0,			   // Geometry shader (optional)
		fShader = glCreateShader(GL_FRAGMENT_SHADER);	// Fragment shader		

	NAME_OBJECT(GL_SHADER, vShader, vertex);
	if (tessEval) NAME_OBJECT(GL_SHADER, teShader, tessEval);
	if (tessCtrl) NAME_OBJECT(GL_SHADER, tcShader, tessCtrl);
	if (geometry) NAME_OBJECT(GL_SHADER,  gShader, geometry);
	NAME_OBJECT(GL_SHADER, fShader, fragment);

	GLuint program = glCreateProgram();

	glShaderBinary(1, &vShader, GL_SHADER_BINARY_FORMAT_SPIR_V, vSrc, strlen(vSrc));
	glSpecializeShader(vShader, "main", 0, NULL, NULL); 
	glAttachShader(program, vShader);

	if (tessEval)
	{
		glShaderBinary(1, &teShader, GL_SHADER_BINARY_FORMAT_SPIR_V, teSrc, strlen(teSrc));
		glSpecializeShader(teShader, "main", 0, NULL, NULL);
		glAttachShader(program, teShader);
	}
	if (tessCtrl)
	{
		glShaderBinary(1, &tcShader, GL_SHADER_BINARY_FORMAT_SPIR_V, tcSrc, strlen(tcSrc));
		glSpecializeShader(tcShader, "main", 0, NULL, NULL);
		glAttachShader(program, tcShader);
	}
	if (geometry)
	{
		glShaderBinary(1, &geometry, GL_SHADER_BINARY_FORMAT_SPIR_V, gSrc, strlen(gSrc));
		glSpecializeShader(gShader, "main", 0, NULL, NULL);
		glAttachShader(program, gShader);
	}

	glShaderBinary(1, &fShader, GL_SHADER_BINARY_FORMAT_SPIR_V, fSrc, strlen(fSrc));
	glSpecializeShader(fShader, "main", 0, NULL, NULL);
	glAttachShader(program, fShader);

	
	GLint isLinked = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	if (!isLinked)
	{
		glDeleteProgram(program);
		program = 0;
	}

	cleanupSource(vSrc);
	cleanupSource(teSrc);
	cleanupSource(tcSrc);
	cleanupSource(teSrc);
	cleanupSource(gSrc);
	cleanupSource(fSrc);

	detachDeleteShader(program, vShader);
	detachDeleteShader(program,teShader);
	detachDeleteShader(program,tcShader);
	detachDeleteShader(program, gShader);
	detachDeleteShader(program, fShader);

	return program;
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



float triangle[] = {
/*	X		Y	Z		R	G	B	*/
	0,	   .5,	0,		1,	0,	0,
	-.5, -.25,	0,		0,	1,	0,
	.5,	 -.25,	0,		0,	0,	1,
};
void triangletest() 
{
	GLuint program = glCreateProgram();
	NAME_OBJECT(GL_PROGRAM, program, "Default Progam");

	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	NAME_OBJECT(GL_VERTEX_ARRAY, vao, "triangle Vertex Array");

	GLuint
		vbo = 0,
		ebo = 0;
	glCreateBuffers(1, &ebo);
	NAME_OBJECT(GL_BUFFER, ebo, "Unused Element Buffer");

	glCreateBuffers(1, &vbo);
	NAME_OBJECT(GL_BUFFER, vbo, "<Triangle Vertex Buffer>");

	uint32_t ixBind = 0;

	glNamedBufferData(vbo, sizeof triangle, NULL, GL_STATIC_DRAW);
	glNamedBufferSubData(vbo, 0L, sizeof triangle, triangle);

	glVertexArrayVertexBuffer(vao, ixBind, vbo, 0L, 6 * sizeof(float));

	//Position (location = 0)
	glVertexArrayAttribBinding(vao, 0, ixBind);
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0L);
	glEnableVertexArrayAttrib(vao, 0);

	//RGB (location = 1)
	glVertexArrayAttribBinding(vao, 1, ixBind);
	glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
	glEnableVertexArrayAttrib(vao, 1);

	//Make Triangle Shader
	{
		GLuint
			vShader = makeShader(GL_VERTEX_SHADER, "default.vert"),
			fShader = makeShader(GL_FRAGMENT_SHADER, "default.frag");
		assert(vShader != 0);
		assert(fShader != 0);

		glAttachShader(program, vShader);
		glAttachShader(program, fShader);

		glLinkProgram(program);

		GLint isLinked = false;
		glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
#ifndef glDebugMessageCallback
		if (!isLinked)
		{
			GLuint len = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
			assert(len != 0);
			const char* msg = malloc(len * sizeof * msg);
			glGetProgramInfoLog(program, len, NULL, msg);

			OutputDebugStringA(msg);
			free(msg);
		}
#endif // !glDebugMessageCallback
		if (!isLinked)
		{
			glDeleteProgram(program);
			return -1;
		}

		// Always delete after successful linkage
		glDetachShader(program, vShader);
		glDeleteShader(vShader);
		glDetachShader(program, fShader);
		glDeleteShader(fShader);
	}

	//loop

	struct Camera cam = { 0 };
	glUseProgram(program);
	//update matrix uniform by multiplying camera's projection*view matrix * model's matrix (=E)
	{
		GLuint matrixLocation = glGetUniformLocation(program, "u_matrix");
		//model matrix
		mat4 matrix;
		mat4x4_mul(matrix, cam.matrix, mat4x4_identity(matrix));
		glProgramUniformMatrix4fv(program, matrixLocation, 1, GL_FALSE, matrix);
	}
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	//Cleanup
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);
}

static inline GLuint activate(GLuint program)
{
	static GLuint activeProgram = 0;

	if (program)
	{
		activeProgram = program;
		glUseProgram(program);
	}

	return activeProgram;
}


/**
	@brief  Crox Application entry point
	@param  ctx  - 
	@param  argC - 
	@param  argV - 
	@param  penv - 
	@retval      - 
**/
int main(_In_ NkContext* ctx, _In_ uint32_t argC, _In_ wchar_t** argV, _In_ wchar_t** penv)
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


	GLuint program = glCreateProgram();
	NAME_OBJECT(GL_PROGRAM, program, "<Basic 3D Program>");
	//setup program using <3D.vert> + <3D.frag>
	{
		GLuint
			vShader = makeShader(GL_VERTEX_SHADER, "3D.vert"),
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



	GLuint vao;
	glCreateVertexArrays(1, &vao);
	NAME_OBJECT(GL_VERTEX_ARRAY, vao, "<Cube Vertex Array>");
	assert(vao != 0);

	uint32_t vertexCount;
	uint32_t indexCount;

	GLuint vbo;
	glCreateBuffers(1, &vbo);
	NAME_OBJECT(GL_BUFFER, vbo, "<Cube Buffer>");
	assert(vbo != 0);

	GLuint ebo;
	glCreateBuffers(1, &ebo);
	NAME_OBJECT(GL_BUFFER, ebo, "<Mesh Indices>");
	assert(ebo != 0);

	assert(vbo != 0);
	//Read <cube.obj> into vbo
	{
		FILE* file;
		fopen_s(&file, "monkey.obj", "r");
		assert(file != NULL);

		struct WavefrontMesh mesh = wavefront_read(file);
		vertexCount = arrlen(mesh.vertices);
		indexCount = arrlen(mesh.indices);
		fclose(file);

		assert(mesh.vertices != NULL);
		assert(vertexCount != 0);

		glNamedBufferData(vbo,			sizeof(struct Vertex) * vertexCount,	NULL, GL_STATIC_DRAW);
		glNamedBufferSubData(vbo, 0L,	sizeof(struct Vertex) *vertexCount,		mesh.vertices);
		
		glNamedBufferData(ebo, sizeof(uint32_t) * indexCount, NULL, GL_STATIC_DRAW);
		glNamedBufferSubData(ebo, 0L, sizeof(uint32_t) * indexCount, mesh.indices);
		
		arrfree(mesh.vertices);
		arrfree(mesh.indices);
	}

	
	GLuint ixBind = 0;
	size_t offset = 0L;
	size_t begin  = 0L;
	size_t stride = sizeof(struct Vertex);
	glVertexArrayVertexBuffer(vao, ixBind, vbo, offset, stride);

	glVertexArrayElementBuffer(vao, ebo);

	//layout(location = 0) in vec3 aXYZ
	glVertexArrayAttribBinding(vao, 0, ixBind);
	glVertexArrayAttribFormat (vao, 0, 3, GL_FLOAT, false, begin);
	glEnableVertexArrayAttrib (vao, 0);
	


	

	vec3 pos = {0,0,-10};
	vec3 dir = {0,0,1};
	const vec3 UP = { 0,1,0 };
	float speed = .1f;
	struct Camera cam = { 0 };
	mat4x4_perspective(cam._projection, M_PI / 4, (float)width / height, .1, 100);
	setView(&cam, pos, dir);


	

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearColor(0, 0, 0, 1.0f);


	platform_show(ctx);
	int result = 0;
	while (running)
	{
#pragma region Input Handling
		nk_input_begin(ctx);
		running = platform_pollMessages(ctx, &result);
		if (!running) break; // Avoid swapping deleted buffers


		float scrollDelta = ctx->input.mouse.scroll_delta.y;
		if (scrollDelta)
		{
			pos[2] += scrollDelta / 10;
			setView(&cam, pos, dir);
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
		setView(&cam, pos, dir);
		;
		nk_input_end(ctx);
#pragma endregion

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		


		glUseProgram(program);
	
		mat4 matrix;//model matrix
		mat4x4_mul(matrix, cam.matrix, mat4x4_identity(matrix)); //matrix = cam's matrix * E
		glProgramUniformMatrix4fv(program, glGetUniformLocation(program, "u_matrix"), 1, GL_FALSE, matrix);
	
		glBindVertexArray(vao);
		//glDrawArrays(GL_TRIANGLES, 0, vertexCount);
		glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, NULL);

		glBindVertexArray(0);

		BOOL swapSuccess = platform_swapBuffers(ctx);
		assert(swapSuccess);
	}

	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteProgram(program);

	return result;
}
