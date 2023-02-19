/*******************************************************************************

	@file    nuklear.h
	@brief   Nuklear defines needed globaly
	@details ~
	@author  Jakob Kristersson <jakob.kristerrson@bredband.net> [Kss0N]
	@date    17.02.2023

*******************************************************************************/
#pragma once

#pragma once

#define NK_NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_COMMAND_USERDATA

#include <nuklear.h>

typedef struct nk_context NkContext;