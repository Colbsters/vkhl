#pragma once

#ifndef VKHL_GLOBALS_HPP
#define VKHL_GLOBALS_HPP

#include <vulkan/vulkan_core.h>

#include <optional>

#include "Definitions.h"

namespace vkhl
{
	VKHL_INLINE_VAR std::optional<VkAllocationCallbacks> g_allocator = std::nullopt;
}

#endif
