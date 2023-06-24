/*******************************************************************************

    @file    framework_crt.h
    @brief   Windows stdlib runtme defines and includes
    @details ~
    @author  Jakob Kristersson <jakob.kristerrson@bredband.net> [Kss0N]
    @date    18.02.2023

*******************************************************************************/
#pragma once
#define _CRT_USE_WINAPI_FAMILY_DESKTOP_APP

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#endif // _DEBUG


#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
//#include <stdatomic.h>
#include <stdbool.h>

#include <uchar.h>
#include <wchar.h>
#include <tchar.h>
#include <float.h>

#include <malloc.h>

#include <crtdbg.h>

#ifdef _DEBUG
#define assert(expr) if(!(expr)) __debugbreak();
#else
#define assert(expr) 
#endif // _DEBUG
