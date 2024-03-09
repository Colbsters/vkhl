#pragma once

#ifndef VKHL_PHYSICALDEVICE_HPP
#define VKHL_PHYSICALDEVICE_HPP

#include <vulkan/vulkan_core.h>

#include <span>
#include <utility>
#include <optional>

#include "Definitions.h"
#include "Globals.hpp"
#include "Error.hpp"
#include "Common.hpp"

#ifdef VKHL_INCLUDE_IMPLEMENTION

#include <vulkan/vk_enum_string_helper.h>
#include <vector>

#endif // VKHL_INCLUDE_IMPLEMENTION

namespace vkhl
{
	using PhysicalDeviceSelectionPredicateFunc = bool(*)(VkPhysicalDevice device, void* usrPtr);

	struct PhysicalDeviceSelectionPredicate
	{
		PhysicalDeviceSelectionPredicateFunc func;
		void* usrPtr;
	};

	using QueueFamilySelectionPredicateFunc = bool(*)(VkPhysicalDevice device, uint32_t queueFamilyIndex, void* usrPtr);

	struct QueueFamilySelectionPredicate
	{
		QueueFamilySelectionPredicateFunc func;
		void* usrPtr;
	};

	struct PhysicalDeviceQueueFamilySelectionInfo
	{
		uint32_t minQueueCount;				// 0 is equivilent to 1
		uint32_t minTimestampBits;
		VkExtent3D maxMinImageGranularity;	// 0 means no maximum

		FeatureRequirement graphics;
		FeatureRequirement compute;
		FeatureRequirement transfer;
		FeatureRequirement sparseBinding;

		std::span<QueueFamilySelectionPredicate> customPredicates;
	};

	struct PhysicalDeviceSelectionInfo
	{
		std::span<PhysicalDeviceQueueFamilySelectionInfo> queueFamilyInfos;
		std::span<PhysicalDeviceSelectionPredicate> customPredicates;
	};

	struct PhysicalDeviceQueueFamilyInfo
	{
		VkQueueFamilyProperties properties;
	};

	struct PhysicalDeviceInfo
	{
		std::vector<PhysicalDeviceQueueFamilyInfo> queueFamilies; // Array of all queue families on device
	};

	// Selects a VkPhysicalDevice based on some features, limits, and custom predicates.
	// Params:
	//	instance = Vulkan instance
	//	selectionInfo = A PhysicalDeviceSelectionInfo struct which describes how to select the physical device
	//	deviceOut -> VkPhysicalDevice
	//	queueFamiliesOut -> An array of uint32_t queue indices which is >= the size of selectionInfo.queueFamilyInfos
	//	infoOut (optional) -> A PhysicalDeviceInfo struct that contains info about the selected device
	VKHL_INLINE SmartResult SelectPhyicalDevice(VkInstance instance, const PhysicalDeviceSelectionInfo& selectionInfo, VkPhysicalDevice* deviceOut, uint32_t* queueFamiliesOut, PhysicalDeviceInfo* infoOut);

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

	VKHL_INLINE SmartResult SelectPhyicalDevice(VkInstance instance, const PhysicalDeviceSelectionInfo& selectionInfo, VkPhysicalDevice* deviceOut, uint32_t* queueFamiliesOut, PhysicalDeviceInfo* infoOut)
	{
		VkResult result = VK_SUCCESS;

		uint32_t deviceCount;
		CHECK_VK_CALL(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr),
			"Failed to get number of physical devices with error %s\n");
		std::vector<VkPhysicalDevice> devices{ deviceCount };
		CHECK_VK_CALL(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()),
			"Failed to get physical devices with error %s\n");

		for (auto device : devices)
		{
			uint32_t queueFamilyCount;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

			// Get queue indices
			bool queueComplete = true;
			std::vector<uint32_t> queueIndices;
			queueIndices.reserve(selectionInfo.queueFamilyInfos.size());

			for (auto queueInfo : selectionInfo.queueFamilyInfos)
			{

				uint32_t index = 0;
				for (const auto& queueFamily : queueFamilies)
				{
					// Is this queue good?
					if (((queueInfo.graphics == RequireFeature) ? (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) : true) &&
						((queueInfo.transfer == RequireFeature) ? (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) : true) &&
						((queueInfo.compute == RequireFeature) ? (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) : true) &&
						((queueInfo.sparseBinding == RequireFeature) ? (queueFamily.queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) : true) &&
						((queueInfo.maxMinImageGranularity.width == 0) || (queueInfo.maxMinImageGranularity.width >= queueFamily.minImageTransferGranularity.width)) &&
						((queueInfo.maxMinImageGranularity.height == 0) || (queueInfo.maxMinImageGranularity.height >= queueFamily.minImageTransferGranularity.height)) &&
						((queueInfo.maxMinImageGranularity.depth == 0) || (queueInfo.maxMinImageGranularity.depth >= queueFamily.minImageTransferGranularity.depth)) &&
						(queueInfo.minQueueCount <= queueFamily.queueCount) &&
						(queueInfo.minTimestampBits <= queueFamily.timestampValidBits))
					{
						// Check custom predicates
						bool allGood = true;
						for (const auto& predicate : queueInfo.customPredicates)
						{
							if (!predicate.func(device, index, predicate.usrPtr))
							{
								allGood = false;
								break;
							}
						}

						if (allGood)
						{
							queueIndices.push_back(index);
							break;
						}
					}

					index++;
				}

				if (index == queueFamilies.size())
				{
					queueComplete = false;
					break;
				}
			}

			if (!queueComplete)
				continue;

			// Check custom predicates
			bool deviceGood = true;
			for (const auto& predicate : selectionInfo.customPredicates)
			{
				if (!predicate.func(device, predicate.usrPtr))
				{
					deviceGood = false;
					break;
				}
			}

			if (!deviceGood)
				continue;

			// Device
			*deviceOut = device;
			// Queue family indices
			std::memcpy(queueFamiliesOut, queueIndices.data(), queueIndices.size() * sizeof(uint32_t));

			// Device info:
			// Queue infos
			infoOut->queueFamilies.reserve(queueFamilyCount);
			for (const auto& familyInfo : queueFamilies)
				infoOut->queueFamilies.push_back({ familyInfo });

			return VK_SUCCESS;
		}

		return VK_ERROR_INITIALIZATION_FAILED;
	}

#undef CHECK_VK_CALL
#endif // VKHL_INCLUDE_IMPLEMENTION
}

#endif
