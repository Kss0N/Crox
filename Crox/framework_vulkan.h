#pragma once

#ifndef _WIN32
#include "framework_winapi.h"
#endif

#define VK_USE_PLATFORM_WIN32_KHR


#include <Volk/volk.h>
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>