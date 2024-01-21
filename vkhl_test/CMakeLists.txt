cmake_minimum_required(VERSION 3.12)

add_executable(vkhl_test "src/main.cpp")

set_target_properties(vkhl_test PROPERTIES CXX_STANDARD 20)

# Link library
target_link_libraries(vkhl_test PRIVATE vkhl)

# Include header files
target_include_directories(vkhl_test PRIVATE "vkhl/include")

