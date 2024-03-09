cmake_minimum_required(VERSION 3.12)

add_library(vkhl "src/vkhl.cpp" "include/vkhl/vkhl.hpp" "include/vkhl/Definitions.h" "include/vkhl/Error.hpp" "include/vkhl/Globals.hpp" "include/vkhl/Defer.hpp" "include/vkhl/Instance.hpp" "include/vkhl/Common.hpp" "include/vkhl/PhysicalDevice.hpp")

set_target_properties(vkhl PROPERTIES CXX_STANDARD 20)

# Find Vulkan
if (DEFINED VULKAN_SDK_PATH)
	set(ENV{VULKAN_SDK} VULKAN_SDK_PATH)
endif()
find_package(Vulkan REQUIRED)


# Link libraries
target_link_libraries(vkhl PUBLIC ${Vulkan_LIBRARIES})

# Include header files from vulkan and from our include directories
target_include_directories(vkhl PUBLIC "$ENV{VULKAN_SDK}/Include" "include")
