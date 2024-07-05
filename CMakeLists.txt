cmake_minimum_required(VERSION 3.5)

project(VkSkeletalAnimationExample LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

add_executable(VkSkeletalAnimationExample WIN32
	Main.cpp
	external/imgui/imgui.cpp
	external/imgui/imgui_draw.cpp
	external/imgui/imgui_demo.cpp
	external/imgui/imgui_widgets.cpp
	external/imgui/imgui_tables.cpp
	external/imgui/backends/imgui_impl_win32.cpp
	external/imgui/backends/imgui_impl_vulkan.cpp
)

include(GNUInstallDirs)
install(TARGETS VkSkeletalAnimationExample
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

target_include_directories(VkSkeletalAnimationExample PUBLIC "${Vulkan_INCLUDE_DIR}")
target_include_directories(VkSkeletalAnimationExample PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/imgui")
target_include_directories(VkSkeletalAnimationExample PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/external/tinygltf")

target_link_libraries(VkSkeletalAnimationExample PUBLIC "Shcore.lib")

target_compile_definitions(VkSkeletalAnimationExample PUBLIC APP_SOURCE_PATH="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(VkSkeletalAnimationExample PUBLIC VK_NO_PROTOTYPES)
