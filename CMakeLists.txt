cmake_minimum_required(VERSION 3.20)
project(vulkan LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Stb REQUIRED)  
find_package(tinyobjloader CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable(vulkan
    main.cpp
    src/Buffers.cpp
    src/Camera.cpp
    src/CommandBuffers.cpp
    src/DebugUtils.cpp
    src/Descriptors.cpp
    src/GraphicsPipeline.cpp
    src/IBL.cpp
    src/ImageResources.cpp
    src/Model.cpp
    src/Picking.cpp
    src/Renderer.cpp
    src/SwapChain.cpp
    src/TriangleApplication.cpp
    src/VulkanDevice.cpp
    src/VulkanInstance.cpp
    src/VmaImplimentation.cpp
    src/VulkanAllocator.cpp
    src/ImGuiLayer.cpp
    src/Skybox.cpp
)

target_include_directories(vulkan PRIVATE
    src
    ${Stb_INCLUDE_DIR}
)

target_link_libraries(vulkan PRIVATE
    Vulkan::Vulkan
    glfw
    glm::glm
    tinyobjloader::tinyobjloader 
    GPUOpen::VulkanMemoryAllocator
    imgui::imgui
)
