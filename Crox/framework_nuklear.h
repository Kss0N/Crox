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
#define NK_INCLUDE_STANDARD_BOOL
#define NK_INCLUDE_FIXED_TYPES

#include <nuklear.h>

typedef struct nk_buffer						NkBuffer;
typedef struct nk_allocator						NkAllocator;
typedef struct nk_command_buffer				NkCommandBuffer;
typedef struct nk_draw_command					NkDrawCommand;
typedef struct nk_convert_config				NkConvertConfig;
typedef struct nk_style_item					NkStyleItem;
typedef struct nk_text_edit						NkTextEdit;
typedef struct nk_draw_list						NkDrawList;
typedef struct nk_user_font						NkUserFont;
typedef struct nk_panel							NkPanel;
typedef struct nk_context						NkContext;
typedef struct nk_draw_vertex_layout_element	NkDrawVertexLayoutElement;
typedef struct nk_style_button					NkStyleButton;
typedef struct nk_style_toggle					NkStyleToggle;
typedef struct nk_style_selectable				NkStyleSelectable;
typedef struct nk_style_slide					NkStyleSlide;
typedef struct nk_style_progress				NkStyleProgress;
typedef struct nk_style_scrollbar				NkStyleScrollbar;
typedef struct nk_style_edit					NkStyleEdit;
typedef struct nk_style_property				NkStyleProperty;
typedef struct nk_style_chart					NkStyleChart;
typedef struct nk_style_combo					NkStyleCombo;
typedef struct nk_style_tab						NkStyleTab;
typedef struct nk_style_window_header			NkStyleWindowHeader;
typedef struct nk_style_window					NkStyleWindow;