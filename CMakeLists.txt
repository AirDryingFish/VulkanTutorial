cmake_minimum_required(VERSION 3.10.0)
project(vulkan VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(vulkan main.cpp)

target_include_directories(vulkan PRIVATE
    E:/mingw64/include
    E:/glm
    C:/VulkanSDK/1.4.321.1/Include
)

target_link_directories(vulkan PRIVATE
    E:/mingw64/lib
    C:/VulkanSDK/1.4.321.1/Lib
)

target_link_libraries(vulkan PRIVATE
    glfw3
    vulkan-1
)