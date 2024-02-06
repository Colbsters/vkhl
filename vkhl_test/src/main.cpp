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

	vkhl::CreateInstance({
		.appName = "vkhl test",
		.engineName = "vkhl test engine",
		.appVersion = vkhl::MakeVersion(1, 0),
		.engineVersion = vkhl::MakeVersion(1, 0),
		.minApiVersion = vkhl::MakeVersion(1, 1),
		.layers = g_instanceLayers
		}, instance, &instanceInfo).Reset();

	vkhl::Defer deferDestroyInst([instance](){
			vkhl::DestroyInstance(instance);
		});





	return 0;
}
