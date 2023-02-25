#include "framework_winapi.h"
#include "framework_nuklear.h"
#include "framework_crt.h"

#include <glad/gl.h>
#include <glad/wgl.h>
#include "resource.h"
#include "Crox.h"
#include "Platform.h"


#ifdef __cplusplus
extern "C"
#endif // __cplusplus
_declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;




static int __CRTDECL crtAllocHook(
	_In_		int		eAllocType,		// {_HOOK_ALLOC, _HOOK_REALLOC, _HOOK_FREE}
	_In_opt_	void* pUserData,		// null when aAllocType in  {_HOOK_ALLOC, _HOOK_REALLOC}
	_In_		size_t	size,
	_In_		int		eBlockType,		// {_FREE_BLOCK, _NORMAL_BLOCK, _CRT_BLOCK, _IGNORE_BLOCK, _CLIENT_BLOCK, _MAX_BLOCKS}
	_In_		long	requestNumber,	// ID
	_In_	unsigned char const* file,	//__FILE__ 
	_In_		int		line);

static void __CRTDECL crtDumpClientHook(
	_In_		void* userdata,
	_In_		size_t	size);

static int __CRTDECL onCrtExit(void);

static void __cdecl purecallHandler(void);

static void __cdecl invalidParamHandler(
	_In_		LPCWSTR		expr,	
	_In_		LPCWSTR		func,
	_In_		LPCWSTR		file,
	_In_		uint32_t	line,
	_In_opt_	uintptr_t	reserved);

static int __CRTDECL crtReportHook(_In_ int eType, _In_ LPWSTR msg, _In_ int* ret);

static BOOL loadOpenGL(_In_ HINSTANCE hInstance);

static LRESULT APIENTRY setupWndProc(_In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM, _In_ LPARAM);
static LRESULT APIENTRY mainWndProc(_In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM, _In_ LPARAM);



struct PlatformResources
{
	HWND hMainWnd;
	HINSTANCE hInstance;
	HACCEL hAccel;
	void* aux;
};
inline HWND		getMainWnd(NkContext* ctx)
{
	return ((struct PlatformResources*)ctx->userdata.ptr)->hMainWnd;
}
inline HACCEL	getAccel(NkContext* ctx)
{
	return ((struct PlatformResources*)ctx->userdata.ptr)->hAccel;
}

/**
	@brief  Application Entry point
	@param  hInstance     - 
	@param  hPrevInstance - 
	@param  lpCmdline     - 
	@param  nCmdShow      - 
	@retval               - 0 on success
**/
extern APIENTRY _tWinMain(
	_In_		HINSTANCE	hInstance,
	_In_opt_	HINSTANCE	hPrevInstance,
	_In_		LPTSTR		lpCmdline,
	_In_		int			nCmdShow)
	//
	// 
	//
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
	_CrtSetAllocHook(crtAllocHook);
	_CrtSetDumpClient(crtDumpClientHook);
	_CrtSetReportHookW2(_CRT_WARN, crtReportHook);
	_onexit(onCrtExit);
#ifdef _DEBUG
	_set_invalid_parameter_handler(invalidParamHandler);
	//_set_purecall_handler(purecallHandler); //TODO: find out what this does
#endif // _DEBUG
	//TODO: locales

	BOOL glInitialized = loadOpenGL(hInstance);
	assert(glInitialized);

#ifdef _DEBUG
	gladInstallGLDebug();
	gladInstallWGLDebug();
#endif // _DEBUG


	const size_t NAMEBUF_SIZE = 128;
	LPTSTR namebuf = malloc(NAMEBUF_SIZE * sizeof * namebuf);
	LoadString(hInstance, IDS_MAINWNDCLS, namebuf, NAMEBUF_SIZE);
	HICON mainIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CROX));

	WNDCLASSEX wcex = {
		.cbSize = sizeof wcex,
		.style = CS_OWNDC |CS_DBLCLKS | CS_VREDRAW |CS_HREDRAW,
		.lpfnWndProc = setupWndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = mainIcon,
		.hCursor = LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)),
		.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH),
		.lpszMenuName = NULL,
		.lpszClassName = namebuf,
		.hIconSm = mainIcon,
	};
	const DWORD EX_WIN_STYLES = 0L;
	const DWORD WIN_STYLES = WS_OVERLAPPEDWINDOW; 
	
	PIXELFORMATDESCRIPTOR pfd;

	int pfdIntAttribs[] = {
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_DRAW_TO_PBUFFER_ARB,GL_TRUE,
		WGL_ACCELERATION_ARB,	WGL_FULL_ACCELERATION_ARB,
		WGL_DOUBLE_BUFFER_ARB,	GL_TRUE,
		
		WGL_PIXEL_TYPE_ARB,		WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,		32,
		WGL_RED_BITS_ARB,		8,
		WGL_GREEN_BITS_ARB,		8,
		WGL_BLUE_BITS_ARB,		8,
		WGL_ALPHA_BITS_ARB,		8,

		WGL_DEPTH_BITS_ARB,		24,
		WGL_STENCIL_BITS_ARB,	8,

		0,0
	};
	float pfdFloatAttribs[] = {
		//currently nothing
		0,0
	};

	int ctxFlags =
#ifdef _DEBUG
		WGL_CONTEXT_DEBUG_BIT_ARB |
#else
		WGL_CONTEXT_OPENGL_NO_ERROR_ARB |
#endif // _DEBUG
		WGL_CONTEXT_ROBUST_ACCESS_BIT_ARB|
		WGL_CONTEXT_CORE_PROFILE_BIT_ARB
		;


	int ctxAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_FLAGS_ARB, ctxFlags,

		WGL_CONTEXT_RELEASE_BEHAVIOR_ARB, WGL_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB,
		WGL_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB, WGL_LOSE_CONTEXT_ON_RESET_ARB,
		0,0
	};



	ATOM mainCls = 0;
	HWND hMainWnd = NULL;
	HDC hDC = NULL;
	HGLRC hCtx = NULL;
	int format = 0;
	int formatsReceivedCount = 0;

	struct nk_context* ctx = malloc(sizeof * ctx);

	BOOL ctxInitialized =
		(mainCls = RegisterClassEx(&wcex)) &&
		LoadString(hInstance, MAKEINTRESOURCE(IDS_TITLE), namebuf, NAMEBUF_SIZE) &&
		(hMainWnd = CreateWindowEx(EX_WIN_STYLES, (LPCTSTR)mainCls, namebuf, WIN_STYLES,
			// X * Y
			CW_USEDEFAULT, 0, //TODO
			// Width * Height
			CW_USEDEFAULT, 0, //TODO
			NULL, NULL, hInstance, ctx)) &&
		(hDC = GetDC(hMainWnd)) &&
		wglChoosePixelFormatARB(hDC, pfdIntAttribs, pfdFloatAttribs, 1, &format, &formatsReceivedCount) &&
		DescribePixelFormat(hDC, format, sizeof pfd, &pfd) &&
		SetPixelFormat(hDC, format, &pfd) &&
		(hCtx = wglCreateContextAttribsARB(hDC, NULL, &ctxAttribs)) &&
		wglMakeContextCurrentARB(hDC, hDC, hCtx);
	free(namebuf);

	assert(ctxInitialized);
	if (hDC)
		ReleaseDC(hMainWnd, hDC);

	int result = 0;
	
	struct nk_user_font font;
	
	if (nk_init_default(ctx, &font))
	{
		LPTSTR pEnv = GetEnvironmentStrings();
		int argC	= 0;
		LPTSTR* argV= CommandLineToArgvW(lpCmdline, &argC);
		
		struct PlatformResources rsc = {
			.hMainWnd = hMainWnd,
			.hInstance = hInstance,
			.hAccel = NULL,
			.aux = NULL,
		};
		nk_set_user_data(ctx, (nk_handle) { .ptr = &rsc });

		result = main(ctx, argC, argV, pEnv);

		if (pEnv)
			FreeEnvironmentStrings(pEnv);
		if (argV)
			LocalFree(argV);

	}



	free(ctx);
	gladLoaderUnloadGL();
	return result;
}

int __CRTDECL crtAllocHook(
	_In_		int		eAllocType,		// {_HOOK_ALLOC, _HOOK_REALLOC, _HOOK_FREE}
	_In_opt_	void*	pUserData,		// null when aAllocType in  {_HOOK_ALLOC, _HOOK_REALLOC}
	_In_		size_t	size,
	_In_		int		eUsage,			// {_FREE_BLOCK, _NORMAL_BLOCK, _CRT_BLOCK, _IGNORE_BLOCK, _CLIENT_BLOCK, _MAX_BLOCKS}
	_In_		long	requestNumber,	// ID
	_In_ unsigned char const* file,		// __FILE__ 
	_In_		int		line)			// __LINE__
{
	// Avoid stack overflow by ignoring internal allocations
	if (eUsage == _CRT_BLOCK)
		return(TRUE); //operation may proceed.

	//TODO
}

void __CRTDECL crtDumpClientHook(
	_In_		void* block,
	_In_		size_t	size)
{
	int eBlockType = _CrtReportBlockType(block);

}

int onCrtExit(void)
{
	//TODO
	return 0;
}

void __cdecl purecallHandler(void)
{
}

void __cdecl invalidParamHandler(
	_In_		LPCWSTR			expression,
	_In_		LPCWSTR			funcName,
	_In_		LPCWSTR			file,		//__FILE__
	_In_		uint32_t		line,		//__LINE__
	_In_opt_	uintptr_t		pReserved)	
{
	DWORD fmFlags =
		FORMAT_MESSAGE_ALLOCATE_BUFFER		|
		FORMAT_MESSAGE_ARGUMENT_ARRAY		|
		//(FORMAT_MESSAGE_MAX_WIDTH_MASK& 128)|
		FORMAT_MESSAGE_FROM_STRING			;
	
	LPWSTR msg = NULL; 
	DWORD_PTR args[] = {(DWORD_PTR)file, (DWORD_PTR)line, (DWORD_PTR)funcName, (DWORD_PTR)expression};
	FormatMessage(fmFlags, 
		L"Invalid Parameter [%s, %d]: routine: %s, expression: %s\n", 
		0, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msg, 0, (va_list)args);
	if (msg != NULL)
	{
		OutputDebugString(msg);
		LocalFree(msg);
	}
	else
	{
		assert(msg != NULL); //quickie TODO: improve it 
	}
		

	exit(-1);
}

int crtReportHook(_In_ int eType, _In_ LPWSTR msg, _In_ int* ret)
{
	OutputDebugString(msg);
	*ret = 0;

	return FALSE;
}

BOOL loadOpenGL(_In_ HINSTANCE hInstance)
{
	WNDCLASS wc = {
		.style = CS_OWNDC,
		.lpfnWndProc = DefWindowProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = hInstance,
		.hIcon = NULL,
		.hCursor = NULL,
		.hbrBackground = NULL,
		.lpszMenuName = NULL,
		.lpszClassName = TEXT("DUMMY WINDOW")
	};

	const DWORD styles = 0L;

	PIXELFORMATDESCRIPTOR pfd;
	memset(&pfd, 0, sizeof pfd);
	pfd.nSize = sizeof pfd;
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.iLayerType = PFD_MAIN_PLANE;


	ATOM aCls	= 0;
	HWND hWnd	= NULL;
	HDC hDC		= NULL;
	HGLRC hCtx	= NULL;
	int format = 0;

	BOOL ctxInitalized =
		(aCls = RegisterClass(&wc))&&
		(hWnd = CreateWindow((LPCTSTR)aCls, TEXT("NONAME"), styles, 
			//X * Y
			CW_USEDEFAULT, 0,
			//Width x Height
			CW_USEDEFAULT, 0,
			NULL, NULL, hInstance, NULL)) &&
		(hDC = GetDC(hWnd)) &&
		(format = ChoosePixelFormat(hDC, &pfd)) &&
		SetPixelFormat(hDC, format, &pfd) &&
		(hCtx = wglCreateContext(hDC)) &&
		wglMakeCurrent(hDC, hCtx) &&
		gladLoaderLoadGL() &&
		gladLoaderLoadWGL(hDC)
		;
	assert(ctxInitalized);

	if (hCtx) 
		if (!wglMakeCurrent(NULL, NULL) && !wglDeleteContext(hCtx))
			DebugBreak();
	if (hDC)
		if (!ReleaseDC(hWnd, hDC))
			DebugBreak();
	if (hWnd)
		if (!DestroyWindow(hWnd))
			DebugBreak();
	if (aCls)
		if (!UnregisterClass((LPCTSTR)aCls, hInstance))
			DebugBreak();

	return ctxInitalized;
}

LRESULT setupWndProc(_In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
//
// make pointer association with the context before any useful messages can be handled.
//
{
	if (msg == WM_NCCREATE)
	{
		LPCREATESTRUCT pCs = (LPCREATESTRUCT)lParam;
		struct nk_context* ctx = (struct nk_context*)pCs->lpCreateParams;

		SetWindowLongPtr(hWnd, GWLP_USERDATA, ctx);
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)&mainWndProc);
		return mainWndProc(hWnd, msg, wParam, lParam);
	}
	else return DefWindowProc(hWnd, msg, wParam, lParam);
}

LRESULT mainWndProc(_In_ HWND hWnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam)
{
	struct nk_context* ctx = GetWindowLongPtr(hWnd, GWLP_USERDATA);
	switch (msg)
	{
	
	case WM_MOUSEMOVE: {

		DWORD xPos = GET_X_LPARAM(lParam);
		DWORD yPos = GET_Y_LPARAM(lParam);

		WORD keys = GET_KEYSTATE_WPARAM(wParam);
		if (keys & MK_CONTROL)
		{
			nk_input_key(ctx, NK_KEY_CTRL, TRUE);
		}
		if (keys & MK_SHIFT)
		{
			nk_input_key(ctx, NK_KEY_SHIFT, TRUE);
		}


		if (keys & MK_LBUTTON)
		{
			//Left Mouse button
			nk_input_button(ctx, NK_BUTTON_LEFT, xPos, yPos, TRUE);
		}
		if (keys & MK_MBUTTON)
		{
			// Middle Mouse button is down
			nk_input_button(ctx, NK_BUTTON_MIDDLE, xPos, yPos, TRUE);
		}
		if (keys & MK_RBUTTON)
		{
			//Right Mouse button is down
			nk_input_button(ctx, NK_BUTTON_RIGHT, xPos, yPos, TRUE);
		}

		if (keys & MK_XBUTTON1)
		{
			//First eXtension Button (forward navigation)
			nk_input_button(ctx, NK_BUTTON_EX1, xPos, yPos, TRUE);
		}
		if (keys & MK_XBUTTON2)
		{
			//Second eX tension button (backward navigation)
			nk_input_button(ctx, NK_BUTTON_EX2, xPos, yPos, TRUE);
		}

		nk_input_motion(ctx, xPos, yPos);

		break;
	}

	case WM_LBUTTONDOWN: case WM_LBUTTONUP:	// Left Mouse button
	case WM_MBUTTONDOWN: case WM_MBUTTONUP:	// Middle Mouse Button
	case WM_RBUTTONDOWN: case WM_RBUTTONUP: // Right Mouse button
	case WM_XBUTTONDOWN: case WM_XBUTTONUP: // eXension Moues  button
	{
		DWORD xPos = GET_X_LPARAM(lParam);
		DWORD yPos = GET_Y_LPARAM(lParam);

		DWORD kfs = GET_KEYSTATE_WPARAM(wParam);
		
		nk_input_key(ctx, NK_KEY_CTRL, kfs & MK_CONTROL);
		nk_input_key(ctx, NK_KEY_SHIFT, kfs & MK_SHIFT);
		
		nk_input_button(ctx, NK_BUTTON_LEFT, xPos, yPos, kfs & MK_LBUTTON);
		nk_input_button(ctx, NK_BUTTON_MIDDLE, xPos, yPos, kfs & MK_MBUTTON);
		nk_input_button(ctx, NK_BUTTON_RIGHT, xPos, yPos, kfs & MK_RBUTTON);
		nk_input_button(ctx, NK_BUTTON_EX1, xPos, yPos, kfs & MK_XBUTTON1);
		nk_input_button(ctx, NK_BUTTON_EX2, xPos, yPos, kfs & MK_XBUTTON2);

	}
	
	case WM_MOUSEWHEEL: {
		DWORD xPos = GET_X_LPARAM(lParam);
		DWORD yPos = GET_Y_LPARAM(lParam);

		WORD keys = GET_KEYSTATE_WPARAM(wParam);
		if (keys & MK_CONTROL)
		{
			nk_input_key(ctx, NK_KEY_CTRL, TRUE);
		}
		if (keys & MK_SHIFT)
		{
			nk_input_key(ctx, NK_KEY_SHIFT, TRUE);
		}


		if (keys & MK_LBUTTON)
		{
			//Left Mouse button
			nk_input_button(ctx, NK_BUTTON_LEFT, xPos, yPos, TRUE);
		}
		if (keys & MK_MBUTTON)
		{
			// Middle Mouse button is down
			nk_input_button(ctx, NK_BUTTON_MIDDLE, xPos, yPos, TRUE);
		}
		if (keys & MK_RBUTTON)
		{
			//Right Mouse button is down
			nk_input_button(ctx, NK_BUTTON_RIGHT, xPos, yPos, TRUE);
		}

		if (keys & MK_XBUTTON1)
		{
			//First eXtension Button (forward navigation)
			nk_input_button(ctx, NK_BUTTON_EX1, xPos, yPos, TRUE);
		}
		if (keys & MK_XBUTTON2)
		{
			//Second eX tension button (backward navigation)
			nk_input_button(ctx, NK_BUTTON_EX2, xPos, yPos, TRUE);
		}

		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

		nk_input_scroll(ctx, (struct nk_vec2) { .x = 0, .y = zDelta });

		break;
	}
	case WM_KEYDOWN: {


		break;
	}
	case WM_CHAR: {
		TCHAR key = (TCHAR)wParam;

		
		WORD flags = HIWORD(lParam);
		WORD scanCode = LOBYTE(flags);
		BOOL isExtended = (flags & KF_EXTENDED) == KF_EXTENDED;

		if (isExtended)
			scanCode = MAKEWORD(scanCode, 0xE0);

		BOOL isDown = (flags & KF_REPEAT) == KF_REPEAT;
		WORD repeatCount = LOWORD(lParam);
		BOOL isReleased = (flags & KF_UP) == KF_UP;
		BOOL isSystem = (flags & KF_ALTDOWN) == KF_ALTDOWN;

		nk_input_char(ctx, key);


		break;
	}

	

	



	case WM_DESTROY:
	{
		PostQuitMessage(0);
		break;
	}
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}


void platform_getDimensions(_In_ NkContext* ctx, _Out_ uint32_t* width, _Out_ uint32_t* height)
{
	HWND hWnd = getMainWnd(ctx);

	RECT cl;
	BOOL success = GetClientRect(hWnd, &cl);
	*width = success ? cl.right - cl.left: 0;
	*height= success ? cl.bottom- cl.top : 0;
}

bool platform_swapBuffers(_In_ NkContext* ctx)
{
	HWND hWnd = getMainWnd(ctx);
	HDC hDC = NULL;
	BOOL success =
		(hDC = GetDC(hWnd)) &&
		SwapBuffers(hDC)
		;
	if (hDC)
		ReleaseDC(hWnd, hDC);

	return success;
}

bool platform_show(_In_ NkContext* ctx)
{
	return ShowWindow(getMainWnd(ctx), SW_SHOW);
}

bool platform_pollMessages(_In_ NkContext* ctx, _Out_ int* status)
{
	HWND hWnd = getMainWnd(ctx);
	HACCEL hAccel = getAccel(ctx);

	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message == WM_QUIT)
		{
			*status = msg.wParam;
			return false;
		}
		
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	*status = 0;

	return true;
}
