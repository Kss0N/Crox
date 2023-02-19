#include "Crox.h"
#include "platform/Platform.h"
#include "framework_crt.h"

#include <glad/gl.h>
#include <glad/wgl.h>
#include <stb_ds.h>
#include <stb_image.h>
#include <stb_include.h>


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

GLuint makeProgramSPIRV()
{


	GLuint program = glCreateProgram();

	//glShaderBinary();
	//glSpecializeShader();



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


int main(
	_In_ NkContext* ctx, _In_ uint32_t argC, _In_ wchar_t** argV, _In_ wchar_t** penv)
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
	NAME_OBJECT(GL_PROGRAM, program, "Default Progam");


	GLuint
		vShader = makeShader(GL_VERTEX_SHADER,  "default.vert"),
		fShader = makeShader(GL_FRAGMENT_SHADER,"default.frag");
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

	glClearColor(0, 0, 0, 1.0f);
	glUseProgram(program);

	platform_show(ctx);

	int result = 0;
	while (running)
	{
		running = platform_pollMessages(ctx, &result);
		if (!running) break; // Avoid swapping deleted buffers

		glClear(GL_COLOR_BUFFER_BIT);



		BOOL swapSuccess = platform_swapBuffers(ctx);
		assert(swapSuccess);
	}
	glDeleteProgram(program);

	return result;
}
