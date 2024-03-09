#pragma once

#ifndef VKHL_INSTANCE_HPP
#define VKHL_INSTANCE_HPP

#include <vulkan/vulkan_core.h>

#include <span>
#include <utility>
#include <optional>
#include <vector>

#include "Definitions.h"
#include "Globals.hpp"
#include "Error.hpp"
#include "Common.hpp"

#ifdef VKHL_INCLUDE_IMPLEMENTION

#include <vulkan/vk_enum_string_helper.h>

#endif // VKHL_INCLUDE_IMPLEMENTION

namespace vkhl
{
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
		std::vector<std::string> extensions;
		std::vector<std::string> layers;
	};

	// instanceOut must point to a VkInstance; infoOut is optional
	VKHL_INLINE SmartResult CreateInstance(const InstanceCreateInfo& createInfo, VkInstance* instanceOut, InstanceInfo* infoOut = nullptr);
	
	// infoOut must point to an InstanceInfo
	VKHL_INLINE SmartResult GetInstanceInfo(InstanceInfo* infoOut);

	// Calls vkDestroyInstance with global allocation callbacks
	VKHL_INLINE void DestroyInstance(VkInstance instance);

#ifdef VKHL_INCLUDE_IMPLEMENTION
// VA_ARGS must start with a printf string, then any extra arguments to send to printf.
// At the end of the printf call there is the stringified result, so make sure that is in the format at the end.
#define CHECK_VK_CALL(call, ...)								\
		result = call;											\
		if (result < 0)											\
		{														\
			PrintError(__VA_ARGS__, string_VkResult(result));	\
			return result;										\
		}

	VKHL_INLINE SmartResult CreateInstance(const InstanceCreateInfo& createInfo, VkInstance* instanceOut, InstanceInfo* infoOut)
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

		std::vector<const char*> layers_cstr;
		layers_cstr.reserve(createInfo.layers.size()); // We might not fill it completely (most likely will, if not close), but we won't need to resize this way

		// For infoOut
		std::vector<std::string> layers;
		if (infoOut)
			layers.reserve(createInfo.layers.size());
		
		// Check layer availablility
		{
			// Get available layers
			uint32_t availableLayerCount = 0;
			CHECK_VK_CALL(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr),
				"Failed to get number of instance layers with error %s\n");

			std::vector<VkLayerProperties> availableLayers{ availableLayerCount };
			CHECK_VK_CALL(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()),
				"Failed to get instance layers with error %s\n");

			if (!createInfo.layers.empty())
			{
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
					{
						layers_cstr.push_back(layer.first);
						if (infoOut)
							layers.push_back(layer.first);
					}

				}
			}
		}

		
		std::vector<const char*> extensions_cstr;
		extensions_cstr.reserve(createInfo.extensions.size()); // We might not fill it completely (most likely will, if not close), but we won't need to resize this way

		// For infoOut
		std::vector<std::string> extensions;
		if (infoOut)
			extensions.reserve(createInfo.extensions.size());

		// Check extension availablility
		{
			// Get available extensions
			uint32_t availableExtCount = 0;
			CHECK_VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, nullptr),
				"Failed to get number of instance extensions with error %s\n");

			std::vector<VkExtensionProperties> availableExtensions{ availableExtCount };
			CHECK_VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, availableExtensions.data()),
				"Failed to get instance extensions with error %s\n");

			// Get available extensions for each of the present layers
			for (auto layer : layers)
			{
				uint32_t layerExtCount = 0;
				CHECK_VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &layerExtCount, nullptr),
					"Failed to get number of layer %s extensions with error %s\n", layer);
				if (layerExtCount > 0)
				{
					// Append the new extensions to the the end of the array (insert at index of old size, a.k.a. availableExtCount)
					availableExtensions.resize(availableExtCount + layerExtCount);
					CHECK_VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &layerExtCount, &availableExtensions[availableExtCount]),
						"Failed to get layer %s extensions with error %s\n", layer);
					availableExtCount += layerExtCount;
				}
			}
		

			if (!createInfo.extensions.empty())
			{
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
					{
						extensions_cstr.push_back(extension.first);
						
						if (infoOut)
							extensions.push_back(extension.first);
					}

				}
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
		instanceInfo.enabledLayerCount = static_cast<uint32_t>(layers_cstr.size());
		instanceInfo.ppEnabledLayerNames = layers_cstr.data();
		instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions_cstr.size());
		instanceInfo.ppEnabledExtensionNames = extensions_cstr.data();
		instanceInfo.pApplicationInfo = &appInfo;

		VkAllocationCallbacks* allocator = nullptr;
		if (g_allocator.has_value())
			allocator = &g_allocator.value();

		result = vkCreateInstance(&instanceInfo, allocator, instanceOut);
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

	VKHL_INLINE SmartResult GetInstanceInfo(InstanceInfo* infoOut)
	{
		VkResult result = VK_SUCCESS;

		// Get version
		uint32_t supportedVersion = VK_VERSION_1_0;
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

		// Get layers
		std::vector<std::string> layers;
		// Check layer availablility
		{
			// Get available layers
			uint32_t availableLayerCount = 0;
			CHECK_VK_CALL(vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr),
				"Failed to get number of instance layers with error %s\n");

			std::vector<VkLayerProperties> availableLayers{ availableLayerCount };
			CHECK_VK_CALL(vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.data()),
				"Failed to get instance layers with error %s\n");

			// Extract the layer names
			layers.reserve(availableLayerCount);
			for (const auto& layerProperties : availableLayers)
				layers.push_back(layerProperties.layerName);
		}

		// Get extensions
		std::vector<std::string> extensions;
		// Check extension availablility
		{
			// Get available extensions
			uint32_t availableExtCount = 0;
			CHECK_VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, nullptr),
				"Failed to get number of instance extensions with error %s\n");

			std::vector<VkExtensionProperties> availableExtensions{ availableExtCount };
			CHECK_VK_CALL(vkEnumerateInstanceExtensionProperties(nullptr, &availableExtCount, availableExtensions.data()),
				"Failed to get instance extensions with error %s\n");

			// Extract the extension names
			extensions.reserve(availableExtCount);
			for (const auto& extProperties : availableExtensions)
				extensions.push_back(extProperties.extensionName);
		}

		infoOut->apiVersion = supportedVersion;
		infoOut->layers = std::move(layers);
		infoOut->extensions = std::move(extensions);

		return VK_SUCCESS;
	}

	VKHL_INLINE void DestroyInstance(VkInstance instance)
	{
		VkAllocationCallbacks* allocator = nullptr;
		if (g_allocator.has_value())
			allocator = &g_allocator.value();

		vkDestroyInstance(instance, allocator);
	}

#undef CHECK_VK_CALL
#endif // VKHL_INCLUDE_IMPLEMENTION
}

#endif
