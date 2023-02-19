#pragma once
#include "framework_nuklear.h"
#include <stdint.h>
#include <stdbool.h>


void platform_getDimensions(_In_ NkContext* ctx, _Out_ uint32_t* width, _Out_ uint32_t* height);

bool platform_swapBuffers(_In_ NkContext* ctx);

bool platform_show(_In_ NkContext* ctx);


//polls messages from the message queue. returns false when a quit message is encountered.
bool platform_pollMessages(_In_ NkContext* ctx, _Out_ int* status);