#include "TriangleApplication.hpp"
#include "DebugUtils.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <unordered_map>
void TriangleApplication::run()
{
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void TriangleApplication::InitWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // GLFW_TRUE：代表可以调整窗口大小
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebufferRizeCallback);
}

void TriangleApplication::InitVulkan()
{
    CreateInstance();
    setupDebugMessenger();
    createSurface();

    pickPhysicalDevice();
    createLogicalDevice();

    createAllocator();

    createSwapChain();
    createImageViews();

    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();

    createCommandPool();
    createColorResources();
    createDepthResources();

    createFramebuffers();

    createTextureImage();
    createTextureImageView();
    createTextureSampler();

    loadModel();

    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffer();

    createDescriptorPool();
    createDescriptorSets();

    createCommandBuffers();
    createSyncObjects();
}

void TriangleApplication::MainLoop()
{
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(device);
}

void TriangleApplication::Cleanup()
{
    cleanupSwapChain();

    vkDestroySampler(device, textureSampler, nullptr);
    // vkDestroyImageView(device, textureImage.imageView, nullptr);

    // // vkDestroyImage(device, textureImage, nullptr);
    // // vkFreeMemory(device, textureImageMemory, nullptr);
    // vmaDestroyImage(allocator, textureImage.image, textureImage.allocation);
    destroyImage(textureImage);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        // vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        // vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        vmaUnmapMemory(allocator, uniformBuffers[i].allocation);
        // vmaDestroyBuffer(allocator, uniformBuffers[i].buffer, uniformBuffers[i].allocation);
        destroyBuffer(uniformBuffers[i]);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

    // vkDestroyBuffer(device, indexBuffer, nullptr);
    // vkFreeMemory(device, indexBufferMemory, nullptr);

    // vkDestroyBuffer(device, vertexBuffer, nullptr);
    // vkFreeMemory(device, vertexBufferMemory, nullptr);

    // vmaDestroyBuffer(allocator, indexBuffer.buffer, indexBuffer.allocation);
    // vmaDestroyBuffer(allocator, vertexBuffer.buffer, vertexBuffer.allocation);
    destroyBuffer(indexBuffer);
    destroyBuffer(vertexBuffer);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    // for (auto framebuffer : swapChainFramebuffers)
    // {
    //     vkDestroyFramebuffer(device, framebuffer, nullptr);
    // }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);
    vmaDestroyAllocator(allocator);
    vkDestroyDevice(device, nullptr);
    if (enableValidationLayers)
    {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);
    glfwTerminate();
}
