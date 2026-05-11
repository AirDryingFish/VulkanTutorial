#pragma once

#include "AppConfig.hpp"
#include "VulkanTypes.hpp"
#include "DeletionQueue.hpp"

#include <string>
#include <vector>

#include <vk_mem_alloc.h>

class TriangleApplication
{
public:
    void run();

private:
    void InitWindow();
    void InitVulkan();

    void CreateInstance();
    void setupDebugMessenger();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo);
    bool checkValidationLayerSupport();
    std::vector<const char *> getRequiredExtensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData);

    void createSurface();
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    void createLogicalDevice();

    void createAllocator();

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
    void createSwapChain();
    void recreateSwapChain();
    static void framebufferRizeCallback(GLFWwindow *window, int width, int height);
    void cleanupSwapChain();
    void createImageViews();

    void createRenderPass();
    static std::vector<char> readFile(const std::string &filename);
    VkShaderModule createShaderModule(const std::vector<char> &code);
    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createFramebuffers();

    void createCommandPool();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);


    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void immediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function);
    

    

    // void createBuffer(
    //     VkDeviceSize size,
    //     VkBufferUsageFlags usage,
    //     VkMemoryPropertyFlags properties,
    //     VkBuffer &buffer,
    //     VkDeviceMemory &bufferMemory);
    // void createBuffer(
    //     VkDeviceSize size,
    //     VkBufferUsageFlags usage,
    //     VkMemoryPropertyFlags properties,
    //     VkBuffer &buffer,
    //     VmaAllocation &allocation);

    AllocatedBuffer createBuffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    // uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();
    void updateUniformBuffer(uint32_t currentImage);

    void createDescriptorPool();
    void createDescriptorSets();

    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    // void createImage(
    //     uint32_t width,
    //     uint32_t height,
    //     uint32_t mipLevels,
    //     VkSampleCountFlagBits numSamples,
    //     VkFormat format,
    //     VkImageTiling tiling,
    //     VkImageUsageFlags usage,
    //     VkMemoryPropertyFlags properties,
    //     VkImage &image,
    //     // VkDeviceMemory &imageMemory
    //     VmaAllocation &allocation
    // );

    AllocatedImage createImage(
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        VkSampleCountFlagBits numSamples,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);

    VkImageView createImageView(
        VkImage image,
        VkFormat format,
        uint32_t mipLevels,
        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void generateMipmaps(VkImage image, VkFormat imageFormat, uint32_t texWidth, uint32_t texHeight, uint32_t mipLevels);

    VkSampleCountFlagBits getMaxUsableSampleCount();

    void createDepthResources();
    VkFormat findSupportedFormat(
        const std::vector<VkFormat> &candidates,
        VkImageTiling tiling,
        VkFormatFeatureFlags features);
    VkFormat findDepthFormat();

    void createColorResources();

    bool hasStencilComponent(VkFormat format);

    void createSyncObjects();
    void drawFrame();
    void MainLoop();
    void Cleanup();

    void destroyBuffer(AllocatedBuffer &buffer);
    void destroyImage(AllocatedImage &image);

    GLFWwindow *window = nullptr;
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;

    VmaAllocator allocator = nullptr;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;

    VkSwapchainKHR swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat{};
    VkExtent2D swapChainExtent{};
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    // VkBuffer vertexBuffer = VK_NULL_HANDLE;
    // // VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    // VmaAllocation vertexBufferAllocation = nullptr;
    // VkBuffer indexBuffer = VK_NULL_HANDLE;
    // // VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    // VmaAllocation indexBufferAllocation = nullptr;

    AllocatedBuffer vertexBuffer;
    AllocatedBuffer indexBuffer;

    // std::vector<VkBuffer> uniformBuffers;
    std::vector<AllocatedBuffer> uniformBuffers;
    // std::vector<VkDeviceMemory> uniformBuffersMemory;
    // std::vector<VmaAllocation> uniformBufferAllocations;
    std::vector<void *> uniformBufferMapped;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptorSets;

    uint32_t mipLevels;

    // VkImage textureImage = VK_NULL_HANDLE;
    // // VkDeviceMemory textureImageMemory = VK_NULL_HANDLE;
    // VmaAllocation textureImageAllocation = nullptr;
    // VkImageView textureImageView = VK_NULL_HANDLE;
    AllocatedImage textureImage;
    VkSampler textureSampler = VK_NULL_HANDLE;

    AllocatedImage depthImage;
    AllocatedImage colorImage;

    // VkImage depthImage = VK_NULL_HANDLE;
    // // VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
    // VmaAllocation depthImageAllocation = nullptr;
    // VkImageView depthImageView = VK_NULL_HANDLE;

    // VkImage colorImage;
    // // VkDeviceMemory colorImageMemory;
    // VmaAllocation colorImageAllocation = nullptr;
    // VkImageView colorImageView;

    DeletionQueue mainDeletionQueue;
    DeletionQueue swapChainDeletionQueue;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    bool framebufferResized = false;
    uint32_t currentFrame = 0;
};
