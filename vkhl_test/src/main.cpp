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
	std::printf("Version: %i.%i.%i\nLayers:\n",
		VK_API_VERSION_MAJOR(instanceInfo.apiVersion), VK_API_VERSION_MINOR(instanceInfo.apiVersion), VK_API_VERSION_PATCH(instanceInfo.apiVersion));
	
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

	return 0;
}
