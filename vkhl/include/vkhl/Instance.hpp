#pragma once

#ifndef VKHL_INSTANCE_HPP
#define VKHL_INSTANCE_HPP

#include <vulkan/vulkan_core.h>

#include <span>
#include <utility>
#include <optional>

#include "Definitions.h"
#include "Globals.hpp"
#include "Error.hpp"
#include "Enums.hpp"

#ifdef VKHL_INCLUDE_IMPLEMENTION

#include <vulkan/vk_enum_string_helper.h>
#include <vector>

#endif // VKHL_INCLUDE_IMPLEMENTION

namespace vkhl
{
	using Version = uint32_t;

	inline Version MakeVersion(uint8_t major, uint8_t minor, uint16_t patch = 0, uint8_t variant = 0)
	{
		return VK_MAKE_API_VERSION(variant, major, minor, patch);
	}

	struct InstanceCreateInfo
	{
		const char* appName;
		const char* engineName;
		Version appVersion;
		Version engineVersion;
		Version minApiVersion; // Will fail if instance does not match or exceed this requirement
		Version maxApiVersion; // Will make an instance up to this version, must be >= minApiVersion or 0, variant must be the same as minApiVersion
		std::span<std::pair<const char*, FeatureRequirement>> extensions;
		std::span<std::pair<const char*, FeatureRequirement>> layers;
	};

	struct InstanceInfo
	{
		Version apiVersion;
		// Please note that the string pointers have the same lifetime as the string pointers passed to CreateInstance,
		// i.e. they go out of scope at the same time
		std::vector<const char*> extensions;
		std::vector<const char*> layers;
	};

	VKHL_INLINE SmartResult CreateInstance(const InstanceCreateInfo& createInfo, VkInstance& instanceOut, InstanceInfo* infoOut = nullptr);

	VKHL_INLINE void DestroyInstance(VkInstance instance);

#ifdef VKHL_INCLUDE_IMPLEMENTION
	VKHL_INLINE SmartResult CreateInstance(const InstanceCreateInfo& createInfo, VkInstance& instanceOut, InstanceInfo* infoOut)
	{
		VkResult result = VK_SUCCESS;

		uint32_t supportedVersion = VK_VERSION_1_0, apiVersion;
		const auto enumerateInstanceVersion = reinterpret_cast<PFN_vkEnumerateInstanceVersion>(vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceVersion"));
		if (enumerateInstanceVersion) // If the function couldn't be found then we are using Vulkan 1.0
		{
			result = enumerateInstanceVersion(&supportedVersion);
			if (result < 0)
			{
				PrintError("Failed to get instance version with error %s\n", string_VkResult(result));
				return result;
			}
		}
		
		if (supportedVersion < createInfo.minApiVersion) // Insufficient version
		{
			PrintError("Instance version isn't sufficient, required %i.%i.%i, but only %i.%i.%i is available\n",
				VK_API_VERSION_MAJOR(createInfo.minApiVersion), VK_API_VERSION_MINOR(createInfo.minApiVersion), VK_API_VERSION_PATCH(createInfo.minApiVersion),
				VK_API_VERSION_MAJOR(supportedVersion), VK_API_VERSION_MINOR(supportedVersion), VK_API_VERSION_PATCH(supportedVersion));
			result = VK_ERROR_INITIALIZATION_FAILED;
		}
		else if (createInfo.maxApiVersion == 0) // Use minApiVersion if not maxApiVersion is provided
			apiVersion = createInfo.minApiVersion;
		else // Use use up to maxApiVersion
			apiVersion = std::min(createInfo.maxApiVersion, supportedVersion);

		std::vector<const char*> extensions;
		extensions.reserve(createInfo.extensions.size()); // We might not fill it completely (most likely will, if not close), but we won't need to resize this way
		// Check extension availablility
		if (!createInfo.extensions.empty())
		{
			uint32_t availableCount;
			vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, nullptr);
			std::vector<VkExtensionProperties> availableExtensions{ availableCount };
			vkEnumerateInstanceExtensionProperties(nullptr, &availableCount, availableExtensions.data());

			for (auto extension : createInfo.extensions)
			{
				const auto iter = std::find_if(availableExtensions.begin(), availableExtensions.end(),
					[&extension](VkExtensionProperties other) {
						return strncmp(extension.first, other.extensionName, VK_MAX_EXTENSION_NAME_SIZE - 1) == 0;
					});

				if (iter == availableExtensions.end())
				{
					// Not found
					if (extension.second == RequireFeature)
					{
						PrintError("Extension %s was required but not found\n", extension.first);
						result = VK_ERROR_EXTENSION_NOT_PRESENT;
					}
					else // extension.second == RequestFeature
						PrintWarning("Extension %s was requested but not found, continuing\n", extension.first);
				}
				else // Found
					extensions.push_back(extension.first);

			}
		}

		std::vector<const char*> layers;
		extensions.reserve(createInfo.layers.size()); // We might not fill it completely (most likely will, if not close), but we won't need to resize this way
		// Check layer availablility
		if (!createInfo.layers.empty())
		{
			uint32_t availableCount;
			vkEnumerateInstanceLayerProperties(&availableCount, nullptr);
			std::vector<VkLayerProperties> availableLayers{ availableCount };
			vkEnumerateInstanceLayerProperties(&availableCount, availableLayers.data());

			for (auto layer : createInfo.layers)
			{
				const auto iter = std::find_if(availableLayers.begin(), availableLayers.end(),
					[&layer](VkLayerProperties other) {
						return strncmp(layer.first, other.layerName, VK_MAX_EXTENSION_NAME_SIZE - 1) == 0;
					});

				if (iter == availableLayers.end())
				{
					// Not found
					if (layer.second == RequireFeature)
					{
						PrintError("Layer %s was required but not found\n", layer.first);
						result = VK_ERROR_EXTENSION_NOT_PRESENT;
					}
					else // layer.second == RequestFeature
						PrintWarning("Layer %s was requested but not found, continuing\n", layer.first);
				}
				else // Found
					layers.push_back(layer.first);

			}
		}

		// Return if the result was set to error (i.e. the instance was not valid)
		// Returning now means I can print all compatiblilty issues, instead of just 1 if I returned on the first error
		if (result < 0)
			return result;

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = createInfo.appName;
		appInfo.applicationVersion = createInfo.appVersion;
		appInfo.pEngineName = createInfo.engineName;
		appInfo.engineVersion = createInfo.engineVersion;
		appInfo.apiVersion = apiVersion;

		VkInstanceCreateInfo instanceInfo{};
		instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		instanceInfo.ppEnabledLayerNames = layers.data();
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceInfo.ppEnabledExtensionNames = extensions.data();
		instanceInfo.pApplicationInfo = &appInfo;

		VkAllocationCallbacks* allocator = nullptr;
		if (g_allocator.has_value())
			allocator = &g_allocator.value();

		result = vkCreateInstance(&instanceInfo, allocator, &instanceOut);
		if (result < 0)
		{
			PrintError("Failed to create instance with error %s\n", string_VkResult(result));
			return result;
		}

		// Write the instance info
		if (infoOut)
		{
			infoOut->apiVersion = apiVersion;
			infoOut->extensions = std::move(extensions);
			infoOut->layers = std::move(layers);
		}

		return VK_SUCCESS;
	}

	// Call vkDestroyInstance with global allocation callbacks
	VKHL_INLINE void DestroyInstance(VkInstance instance)
	{
		VkAllocationCallbacks* allocator = nullptr;
		if (g_allocator.has_value())
			allocator = &g_allocator.value();

		vkDestroyInstance(instance, allocator);
	}
#endif // VKHL_INCLUDE_IMPLEMENTION
}

#endif
