#pragma once
#include "framework_nuklear.h"
#include <stdint.h>
#include <stdbool.h>


void platform_getDimensions(_In_ NkContext* ctx, _Out_ uint32_t* width, _Out_ uint32_t* height);

bool platform_swapBuffers(_In_ NkContext* ctx);

bool platform_show(_In_ NkContext* ctx);


//polls messages from the message queue. returns false when a quit message is encountered.
bool platform_pollMessages(_In_ NkContext* ctx, _Out_ int* status);

typedef struct _Platform_DLL* PlatformDLLHandle;

_Success_(return != NULL) 
PlatformDLLHandle platform_loadLibrary(_In_ NkContext* ctx, _In_z_ const char* libraryPath);

void platform_freeLibrary(_In_ NkContext * ctx, PlatformDLLHandle dll);

_Success_(return != NULL)
void* platform_getProcAdress(PlatformDLLHandle dll, _In_z_ const char* name);