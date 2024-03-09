#include <vkhl/vkhl.hpp>

#include <iostream>
#include <array>

std::pair<const char*, vkhl::FeatureRequirement> g_instanceLayers[] = {
	{ "VK_LAYER_KHRONOS_validation", vkhl::RequestFeature }
};

int main()
{
	VkInstance instance;
	vkhl::InstanceInfo instanceInfo;

	vkhl::GetInstanceInfo(&instanceInfo);
	auto instVersion = vkhl::MakeVersionStruct(instanceInfo.apiVersion);
	std::printf("Version: %i.%i.%i\nLayers:\n", instVersion.major, instVersion.minor, instVersion.patch);
	
	for (auto layer : instanceInfo.layers)
		std::printf("\t%s\n", layer.c_str());

	puts("Extensions:");
	for (auto extension : instanceInfo.extensions)
		std::printf("\t%s\n", extension.c_str());

	vkhl::CreateInstance({
		.appName = "vkhl test",
		.engineName = "vkhl test engine",
		.appVersion = vkhl::MakeVersion(1, 0),
		.engineVersion = vkhl::MakeVersion(1, 0),
		.minApiVersion = vkhl::MakeVersion(1, 1),
		.layers = g_instanceLayers
		}, &instance, &instanceInfo).Reset();

	vkhl::Defer deferDestroyInst([instance](){
			vkhl::DestroyInstance(instance);
		});

	vkhl::PhysicalDeviceQueueFamilySelectionInfo queueInfos[2] = {
		{
			.graphics = vkhl::RequireFeature,
			.compute = vkhl::RequireFeature,
			.transfer = vkhl::RequireFeature
		},
		{
			.transfer = vkhl::RequireFeature
		}
	};

	VkPhysicalDevice physicalDevice;
	uint32_t queueFamilyIndices[2];
	vkhl::PhysicalDeviceInfo physicalDeviceInfo;

	vkhl::SelectPhyicalDevice(instance, {
			.queueFamilyInfos = queueInfos,
		}, &physicalDevice, queueFamilyIndices, &physicalDeviceInfo);

	return 0;
}
