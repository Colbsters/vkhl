#pragma once

#ifndef VKHL_ENUMS_HPP
#define VKHL_ENUMS_HPP

#include <cstdint>

#include <vulkan/vulkan_core.h>

namespace vkhl
{
	enum FeatureRequirement : uint8_t
	{
		DisableFeature = 0,
		RequireFeature = 1, // Errors if not available
		RequestFeature = 2, // Warns if not available
	};

	using Version = uint32_t;

	struct VersionStruct
	{
		uint8_t major;
		uint8_t minor;
		uint16_t patch = 0;
		uint8_t variant = 0;
	};

	inline Version MakeVersion(uint8_t major, uint8_t minor, uint16_t patch = 0, uint8_t variant = 0)
	{
		return VK_MAKE_API_VERSION(variant, major, minor, patch);
	}

	inline Version MakeVersion(VersionStruct version)
	{
		return VK_MAKE_API_VERSION(version.variant, version.major, version.minor, version.patch);
	}

	inline VersionStruct MakeVersionStruct(Version version)
	{
		return {
			static_cast<uint8_t>(VK_API_VERSION_MAJOR(version)),
			static_cast<uint8_t>(VK_API_VERSION_MINOR(version)),
			static_cast<uint16_t>(VK_API_VERSION_PATCH(version)),
			static_cast<uint8_t>(VK_API_VERSION_VARIANT(version))
		};
	}
}

#endif
