#include "TriangleApplication.hpp"

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
SwapChainSupportDetails TriangleApplication::querySwapChainSupport(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;

    // step 1: 查询基本的表面功能
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    // step 2: 查询显卡和窗口表面支持哪些图像格式
    uint32_t formatCount;
    // 获取“有几种格式”
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        // 写入查询到的格式
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    // step 3: 同理，查询显卡和窗口表面支持哪些presentation模式
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR TriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
    // VkSurfaceFormatKHR 结构体条目包含一个format成员和一个colorSpace成员。
    // format成员指定颜色通道和类型。例如：VK_FORMAT_B8G8R8A8_SRGB 表示每个像素有4个8位通道，分别是蓝、绿、红、透明度，并且使用sRGB颜色空间（伽马矫正后的）。
    // colorSpace成员使用VK_COLOR_SPACE_SRGB_NONLINEAR_KHR标志指示是否支持sRGB颜色空间。如果可用，使用sRGB颜色空间可以获得更准确的颜色显示。
    for (const auto &availableForat : availableFormats)
    {
        if (availableForat.format == VK_FORMAT_B8G8R8A8_SRGB && availableForat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableForat;
        }
    }
    // 如果不支持，可以根据格式的好坏对可用格式排名。但通常直接采用第一个指定格式就可以了
    return availableFormats[0];
}

VkPresentModeKHR TriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
{
    for (const auto &availablePresentMode : availablePresentModes)
    {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    {
        // 证明vulkan已经帮我们选好了swapchain的分辨率，直接返回就行了
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    VkExtent2D actualExtent =
        {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)};
    actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
}

void TriangleApplication::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    // 给GPU多一张周转用的画布。
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

    // 限制image最大值来避免申请过多的图像占满GPU内存。某些平台允许的最大值非常大，甚至没有限制，所以如果maxImageCount是0，说明没有限制。
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    // 具体进行swap chain的创建(创建swap chain的图像)
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1; // 指定每个图像有多少层。除非进行立体3D渲染，否则总是1
    // 指定swap chain图像将如何使用。COLOR_ATTACHMENT 意思是"我要直接往上面画东西"，Swap Chain 的 Image 就是最终渲染目标。
    // 如果要后处理，不往swap chain image上画，而是先渲染到一张单独的Image上，做完后处理再把结果拷贝到Swap Chain Image上，这时Swap Chain Image的用途是“被拷贝到”，所以要用VK_IMAGE_USAGE_TRANSFER_DST_BIT。
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // 指定如何处理将在多个队列族中使用的交换链图像。
    // case 1: 同族 + EXCLUSIVE — 最简单最快，大部分情况
    // case 2: 不同族 + CONCURRENT — 简单但稍慢，驱动自动处理
    // case 3: 不同族 + EXCLUSIVE — 最麻烦但最快，手动处理所有权转移，高级优化才会用到
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    // 不同时（少数显卡）：渲染用Graphics队列，显示用Present队列，两个队列族都需要访问同一张Image。这时用CONCURRENT模式，允许多个队列族共享Image，不需要手动所有权转移。性能差但实现简单
    if (indices.graphicsFamily != indices.presentFamily)
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // 图像在多个队列族之间共享，不能独占地属于一个队列族
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    // 相同时（大部分情况）：用EXCLUSIVE模式更快，因为不需要显式地转移图像所有权。Image归一个队列族独占，不需要再求vulkan在队列族之间转移图像所有权了，性能更好。
    // 同一个族就像一个人既能画画又能展示，画完直接挂墙上就行（EXCLUSIVE）。不同族就像画家画完要交给策展人去展示，两个人都需要接触这幅画，所以要设成共享模式（CONCURRENT）。
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // 图像独占地属于一个队列族，访问更快
        createInfo.queueFamilyIndexCount = 0;                    // Optional
        createInfo.pQueueFamilyIndices = nullptr;                // Optional
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // 指定在呈现之前对图像进行的变换。当前不需要变换，直接使用默认值。
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;            // 指定alpha通道的使用方式。当前不需要透明度，直接使用默认值。

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;             // 指定是否允许对不可见的像素进行裁剪。比如被其他窗口遮挡的部分，或者在屏幕外的部分。裁剪掉这些像素可以提高性能，因为不需要渲染它们。
    createInfo.oldSwapchain = VK_NULL_HANDLE; // 这个参数在交换链重创时使用。当前是第一次创建交换链，所以没有旧的交换链，直接设置为VK_NULL_HANDLE。

    // 根据createInfo中的参数创建交换链，并把得到的交换链句柄存储在swapChain变量中，以便后续使用。
    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void TriangleApplication::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0)
    {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();
    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
}

void TriangleApplication::framebufferRizeCallback(GLFWwindow *window, int width, int height)
{
    auto app = reinterpret_cast<TriangleApplication *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void TriangleApplication::cleanupSwapChain()
{
    for (auto framebuffer : swapChainFramebuffers)
    {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    // vkDestroyImageView(device, depthImage.imageView, nullptr);
    // // vkDestroyImage(device, depthImage, nullptr);
    // // vkFreeMemory(device, depthImageMemory, nullptr);
    // vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
    

    // vkDestroyImageView(device, colorImage.imageView, nullptr);
    // // vkDestroyImage(device, colorImage, nullptr);
    // // vkFreeMemory(device, colorImageMemory, nullptr);
    // vmaDestroyImage(allocator, colorImage.image, colorImage.allocation);
    destroyImage(depthImage);
    destroyImage(colorImage);

    for (auto imageView : swapChainImageViews)
    {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void TriangleApplication::createImageViews()
{
    // VkImage本身指示一块GPU内存中的原始数据，vulkan不知道怎么读取它。
    // VkImageView描述了解读方式：把它当2D纹理还是3D纹理还是立方体贴图，读哪些mipmap，读哪些数组层，颜色格式是什么
    // 同一张Image可以有多个ImageView：一张带mipmap的纹理，可以创建一个ImageView只看第0层（全分辨率），再创建一个只看第3层（低分辨率）。】
    // 渲染管线需要用到Image时，必须通过ImageView访问，不能直接用Image
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++)
    {
        // VkImageViewCreateInfo createInfo{};
        // createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        // createInfo.image = swapChainImages[i];
        // // 将图像视为一张一维纹理、二维纹理、三维纹理、立方体贴图等。对于交换链图像来说，直接当成二维纹理就行了。
        // createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        // createInfo.format = swapChainImageFormat;
        // // components允许重新排列颜色通道。比如可以将所有通道映射到红色通道，创建单色纹理
        // // 还可以将常量值0、1映射到某个通道。下面使用默认映射
        // createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        // createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        // createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        // createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        // // subresourceRange描述图像的用途，以及应该访问图像哪个部分
        // // 现在的图像不包含任何mipmapping级别或多层图像
        // createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 图像的哪一部分被视为颜色数据。对于交换链图像来说，只有颜色数据，所以直接设置为VK_IMAGE_ASPECT_COLOR_BIT
        // createInfo.subresourceRange.baseMipLevel = 0;                       // 访问mipmap的哪个级别。现在没有mipmap，所以直接设置为0
        // createInfo.subresourceRange.levelCount = 1;                         // 访问多少个mipmap级别。现在没有mipmap，所以直接设置为1
        // createInfo.subresourceRange.baseArrayLayer = 0;                     // 访问多层图像的哪个数组层。现在没有多层图像，所以直接设置为0
        // createInfo.subresourceRange.layerCount = 1;                         // 访问多少个数组层。现在没有多层图像，所以直接设置为1

        // // 如果要开发立体3D程序，需要创建一个包含多个图层的swap chain。然后通过访问不同图层为每个图像创建多个ImageView，代表左眼右眼的视图
        // // 现在创建图像只用调用一次vkCreateImageView即可
        // if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
        // {
        //     throw std::runtime_error("failed to create image views!");
        // }
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, 1);
    }
}
