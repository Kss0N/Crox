/*******************************************************************************

    @file    framework_winapi.h
    @brief   Includes and defines for winapi
    @details ~
    @author  Jakob Kristersson <jakob.kristerrson@bredband.net> [Kss0N]
    @date    17.02.2023

*******************************************************************************/
#pragma once

#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include <sdkddkver.h>
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#include <winapifamily.h>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <shellapi.h>