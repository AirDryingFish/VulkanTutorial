// #include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr)
    {
        func(instance, debugMessenger, pAllocator);
    }
}

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete()
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

// 查询交换链支持的细节：表面属性、支持的格式、支持的呈现模式 (swap chain 和 surface的兼容性)
struct SwapChainSupportDetails
{

    // 基本surface功能： swapchain中的图像数量、图像最大最小宽高
    VkSurfaceCapabilitiesKHR capabilities;
    // surface格式
    std::vector<VkSurfaceFormatKHR> formats;
    // 可用的 presentation 模式：比如是否支持垂直同步等

    // Present mode 是 swap chain 中最重要的设置，因为它代表了屏幕上显示图像的实际条件。
    //
    // Vulkan 中有 4 种可用的显示模式：
    // * VK_PRESENT_MODE_IMMEDIATE_KHR:
    //      应用程序提交的图像会立即传输到屏幕上，可能导致画面撕裂
    //
    // * VK_PRESENT_MODE_FIFO_KHR (双缓冲垂直同步):
    //      swap chain 是一个队列 (FIFO)，显示器会从队列中取出图像显示，程序将渲染好的图像插入队列后端。如果队列满了，GPU就必须等待不进行渲染了，浪费性能。
    //      这个模式类似于垂直同步 (V-sync)，可以避免画面撕裂。显示器刷新的瞬间被称为 "Vertical Blank"。
    //
    // * VK_PRESENT_MODE_FIFO_RELAXED_KHR:
    //      和 VK_PRESENT_MODE_FIFO_KHR 类似，但如果队列空了，不等下一次垂直刷新，立刻把图显示出来。
    //      可能会导致画面撕裂，因为显示器刷新到一半时画面被换了。但在某些情况下可以减少延迟。
    //
    // * VK_PRESENT_MODE_MAILBOX_KHR (三重缓冲):
    //      和 VK_PRESENT_MODE_FIFO_KHR 类似，但如果队列满了，它不会阻塞应用程序，而是直接用新图像替换已排队的图像。
    //      e.g. 有三张Image：A正在被显示器显示，B在队列里等待，GPU在往C上渲染。C渲染完了之后直接替换队列的B，GPU不等待立刻开始渲染下一帧。显示器席次刷新时取走的是最新的C而不是旧的B
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication
{
public:
    void run()
    {
        InitWindow();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    void InitWindow()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        window = glfwCreateWindow(800, 600, "Vulkan", nullptr, nullptr);
    }
    void InitVulkan()
    {
        CreateInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
    }

    void createSurface()
    {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    // 逻辑设备的创建
    void createLogicalDevice()
    {

        // Step 1: 查询队列族索引： 找到物理设备上支持图形操作的队列族索引
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        // Step 2: 填写队列配置： 声明要几个队列、用哪几个队列族、优先级多少
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;
        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        // Step 3: 声明设备特性：声明要弃用哪些GPU特性，比如这里我们不需要任何特殊的GPU特性，所以就创建一个默认的结构体。
        VkPhysicalDeviceFeatures deviceFeatures{};

        // Step 4: 填写设备配置: 把队列配置、设备特性、需要的拓展等信息填入 VkDeviceCreateInfo 结构体，传给 vkCreateDevice 创建逻辑设备。
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;
        createInfo.pEnabledFeatures = &deviceFeatures;
        // createInfo.enabledExtensionCount = 0;
        // swap chain 是vulkan的一个拓展功能，所以需要在创建logical device的时候告诉vulkan我们需要这个拓展。
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        // Step 5: 创建逻辑设备：调用 vkCreateDevice 创建逻辑设备，并把得到的设备句柄存储在 device 变量中。
        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create logical device!");
        }

        // Step 6: 获取队列句柄：调用 vkGetDeviceQueue 获取图形队列的句柄，并存储在 graphicsQueue 变量中，以便后续使用。
        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    }

    void pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0)
        {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const VkPhysicalDevice &device : devices)
        {
            if (isDeviceSuitable(device))
            {
                physicalDevice = device;
                break;
            }
        }
        if (physicalDevice == VK_NULL_HANDLE)
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        // // multimap allows key to be repeated, so we can store multiple candidates and sort them based on score, then pick the best one.
        // std::multimap<int, VkPhysicalDevice> candidates;
    }

    bool isDeviceSuitable(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices = findQueueFamilies(device);
        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported)
        {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device)
    {
        // return true;
        // step 1: 查询显卡支持哪些拓展，存到 availableExtensions 中。
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        // step 2: 把需要的拓展，比如这里的Swap Chain的拓展 "VK_KHR_swapchain"，放入一个set中
        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        // step 3: 遍历显卡支持的拓展，每找到一个就从set中删除一个，最后如果set是空的，说明显卡支持我们需要的所有拓展。
        for (const auto &extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }
        return requiredExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device)
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

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats)
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

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes)
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

    // swap extent是swap chain图像的分辨率，通常等于窗口大小
    // 大部分情况下，Vulkan 通过 currentExtent 直接告诉你"用这个分辨率"，照抄就行。
    // 特殊情况：某些窗口管理器允许swapchain分辨率和窗口大小不一致。这时候vulkan会把currentExtent的宽高设置为uint32_t最大值0xFFFFFFFF作为信息。意思是“不替你选，自己选”
    // 这时候需要查询自己窗口的实际像素大小，然后限制到minImageExtent和maxImageExtent之间，最后返回这个值。

    // 上面创建glfw窗口使用的width和height是屏幕坐标系，不是真实像素。但vulkan使用的是真实像素，因此swapchain范围也必须以像素为单位指定。
    // 如果使用高DPI显示器，屏幕坐标系不对应像素。如果把高DPI的800x600传给swapchain，实际只用了四分之一的像素，画面会模糊。所以必须用glfwGetFramebufferSize获取真实像素尺寸1600×1200
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities)
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

    void createSwapChain()
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

        // 具体进行swap chain的创建
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

    void createImageViews()
    {
        // VkImage本身指示一块GPU内存中的原始数据，vulkan不知道怎么读取它。
        // VkImageView描述了解读方式：把它当2D纹理还是3D纹理还是立方体贴图，读哪些mipmap，读哪些数组层，颜色格式是什么
        // 同一张Image可以有多个ImageView：一张带mipmap的纹理，可以创建一个ImageView只看第0层（全分辨率），再创建一个只看第3层（低分辨率）。】
        // 渲染管线需要用到Image时，必须通过ImageView访问，不能直接用Image
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            // 将图像视为一张一维纹理、二维纹理、三维纹理、立方体贴图等。对于交换链图像来说，直接当成二维纹理就行了。
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            // components允许重新排列颜色通道。比如可以将所有通道映射到红色通道，创建单色纹理
            // 还可以将常量值0、1映射到某个通道。下面使用默认映射
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            // subresourceRange描述图像的用途，以及应该访问图像哪个部分
            // 现在的图像不包含任何mipmapping级别或多层图像
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // 图像的哪一部分被视为颜色数据。对于交换链图像来说，只有颜色数据，所以直接设置为VK_IMAGE_ASPECT_COLOR_BIT
            createInfo.subresourceRange.baseMipLevel = 0;                       // 访问mipmap的哪个级别。现在没有mipmap，所以直接设置为0
            createInfo.subresourceRange.levelCount = 1;                         // 访问多少个mipmap级别。现在没有mipmap，所以直接设置为1
            createInfo.subresourceRange.baseArrayLayer = 0;                     // 访问多层图像的哪个数组层。现在没有多层图像，所以直接设置为0
            createInfo.subresourceRange.layerCount = 1;                         // 访问多少个数组层。现在没有多层图像，所以直接设置为1

            // 如果要开发立体3D程序，需要创建一个包含多个图层的swap chain。然后通过访问不同图层为每个图像创建多个ImageView，代表左眼右眼的视图
            // 现在创建图像只用调用一次vkCreateImageView即可
            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
    {
        QueueFamilyIndices indices;
        // 取出queue family的数量，存到queueFamilyCount中
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        // 遍历每个queue family，找到一个支持图形操作的队列族就把它的索引存到indices.graphicsFamily中。因为我们现在还没有创建逻辑设备，所以无法查询哪个队列族支持presentation，所以先不管presentation，后面创建完逻辑设备了再来查询presentation支持哪个队列族。
        int i = 0;
        for (const auto &queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport)
            {
                indices.presentFamily = i;
            }

            if (indices.isComplete())
            {
                break;
            }

            i++;
        }

        return indices;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger()
    {
        if (!enableValidationLayers)
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    // 只有一个颜色缓冲区attachment，由swap chain中一张图片表示
    void createRenderPass()
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;   // swap chain图像的格式
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 不使用多重采样

        // LoadOp和storeOp决定如何处理attachment中的数据
        // LoadOp:
        // * VK_ATTACHMENT_LOAD_OP_LOAD: 保留attachment现有内容
        // * VK_ATTACHMENT_LOAD_OP_CLEAR: 在渲染开始时把attachment清空为一个常量值（clear color）
        // * VK_ATTACHMENT_LOAD_OP_DONT_CARE: 现有内容未定义，不关心attachment开始时是什么
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        // StoreOp:
        // * VK_ATTACHMENT_STORE_OP_STORE: 渲染后的内容存储在内存中，稍后可以读取
        // * VK_ATTACHMENT_STORE_OP_DONT_CARE: 渲染后的内容未定义，不关心attachment结束时是什么
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;   // 不使用模板缓冲区，所以不关心它的内容
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // 不使用模板缓冲区，所以不关心它的内容

        // vulkan中的texture和framebuffer由具体特定像素格式的VkImage对象表示，内存中像素的布局可以根据对图像的操作而改变
        // * VK_IMAGE_LAYOUT_UNDEFINED: render pass 开始时不关心图像开始时的布局，渲染开始时图像中的数据会被丢弃
        // * VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: 渲染过程中图像的最佳布局
        // * VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Render pass结束时，自动把图像转换成适合呈现到屏幕的布局。这样Render pass结束后图像就可以直接被显示器使用了。
        // initialLayout和finalLayout的转换由vulkan自动完成，我们只需要告诉vulkan我们不关心初始布局是什么，渲染结束后要把它转换成适合显示的布局就行了。
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // 一个 render pass 里可以有多个 subpass，每个 subpass 是一个渲染步骤，后面的 subpass 可以读取前面 subpass 的输出结果。
        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;                                    // 指定我们要使用第一个attachment
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // subpass使用attachment时的布局

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        // pColorAttachments是关键部分。传入的是一个数组，数组的索引直接对应fragment shader里的layout(location = 0) out vec4 outColor;中的location。
        // 比如如果传入两个attachment，索引分别是0和1，那么fragment shader里就可以有两个输出变量，分别是layout(location = 0) out vec4 outColor0;和layout(location = 1) out vec4 outColor1;。
        // 如果fragment shader里只有一个输出变量，直接写layout(location = 0) out vec4 outColor;就行了，vulkan会自动把它绑定到索引为0的attachment上。
        // 这就是Multiple Render Targets (MRT)技术，可以在一个subpass里同时输出到多张图像上，性能更好。
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    // 读取指定文件中的所有字节，并将他们存储在由vector管理的字节数组中。
    static std::vector<char> readFile(const std::string &filename)
    {
        // ate: 从文件末尾开始读取，这样就可以直接得到文件大小，不需要先读一遍来计算大小了
        // binary: 以二进制模式打开文件，SPIR-V 是二进制数据。如果用文本模式，Windows 会把 \r\n 自动转成 \n，破坏二进制内容。
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        // 光标位置 = 文件字节数
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();
        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char> &code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        // SPIR-V的字节码大小(字节数)
        createInfo.codeSize = code.size();
        // 指向SPIR-V字节码的指针。SPIR-V字节码是一个uint32_t数组，所以需要把char*转换成uint32_t*。
        // 因为SPIR-V字节码的大小必须是4的倍数，所以code.size()一定是4的倍数，reinterpret_cast是安全的。
        createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }
        return shaderModule;
    }

    void createGraphicsPipeline()
    {
        auto vertShaderCode = readFile("shaders/vert.spv");
        auto fragShaderCode = readFile("shaders/frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
        // ------------------------------------------------------------------------------

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        // 指定shader入口函数的名字，SPIR-V允许一个shader模块包含多个入口函数，这个参数告诉vulkan我们要用哪个入口函数。GLSL默认是main，所以SPIR-V也默认是main。
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // 创建动态状态：可以在绘制时无需重新创建管线即可更改的状态。比如视口和裁剪区域经常需要根据窗口大小调整，如果把它们设为动态状态，就不需要在窗口大小改变时重新创建管线了。
        std::vector<VkDynamicState> dynamicStates =
            {
                VK_DYNAMIC_STATE_VIEWPORT,
                VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicStatesInfo{};
        dynamicStatesInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStatesInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicStatesInfo.pDynamicStates = dynamicStates.data();

        // 顶点输入
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        // 数据怎么从buffer里读：每个顶点占多少字节、是逐顶点读还是逐实例读。“一行数据有多宽”
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.pVertexBindingDescriptions = nullptr;
        // 顶点里有哪些属性、怎么拆分
        vertexInputInfo.vertexAttributeDescriptionCount = 0;
        vertexInputInfo.pVertexAttributeDescriptions = nullptr;

        // Input assembly 输入组件
        // * VkPipelineInputAssemblyStateCreateInfo 结构体描述了: 1. 从顶点绘制的几何体类型 2. 是否启用图元重启
        // 1. 几何体类型可以在topology成员中指定：
        // VK_PRIMITIVE_TOPOLOGY_POINT_LIST: 每个顶点都是一个独立的点
        // VK_PRIMITIVE_TOPOLOGY_LINE_LIST: 每两个顶点组成一条线段，不重复使用
        // VK_PRIMITIVE_TOPOLOGY_LINE_STRIP: 每个顶点和前一个顶点组成一条线段，每行的结束顶点用作下一行的起始顶点
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST: 每三个顶点组成一个三角形，不重复使用
        // VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP: 每个顶点和前两个顶点组成一个三角形，每行的结束顶点用作下一行的起始顶点
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
        inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; // 只有在使用带重启的拓扑（比如LINE_STRIP）时才需要这个参数。它允许你在顶点数据中插入一个特殊的索引值，来表示当前图元结束，下一个图元开始。对于TRIANGLE_LIST来说不需要，所以设置为VK_FALSE。

        // Viewports and scissors
        // 视口描述了framebuffer中用于渲染输出的区域，总是介于(0, 0)~(width, height)之间
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        // 这里是实际的像素尺寸，不是屏幕坐标系的尺寸。因为vulkan使用的是真实像素，所以swapchain范围也必须以像素为单位指定。
        viewport.width = (float)swapChainExtent.width;
        viewport.height = (float)swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportStateInfo{};
        viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateInfo.viewportCount = 1;
        viewportStateInfo.pViewports = &viewport;
        viewportStateInfo.scissorCount = 1;
        viewportStateInfo.pScissors = &scissor;

        // Rasterizer 光栅化器
        VkPipelineRasterizationStateCreateInfo rasterizerInfo{};
        rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO;
        // 如果启用，光栅化器会把超出近平面和远平面的片段夹紧到近平面和远平面上，而不是丢弃它们。（clamp depth values）
        // 这样可以避免一些图形失真(阴影贴图有用)，但可能会导致性能下降。对于大多数应用来说，直接丢弃这些片段更简单高效，所以设置为VK_FALSE。
        rasterizerInfo.depthClampEnable = VK_FALSE;
        // 如果启用，光栅化器会跳过所有输出到帧缓冲的阶段，直接丢弃所有图元。这对于仅使用顶点处理阶段进行计算的程序很有用，但对于渲染来说必须设置为VK_FALSE。
        rasterizerInfo.rasterizerDiscardEnable = VK_FALSE;
        // polygonMode决定了几何体片元的生成方式：
        // * VKPOLYGON_MODE_FILL: 用片元填充多边形区域
        // * VKPOLYGON_MODE_LINE: 只用线段描边多边形区域
        // * VKPOLYGON_MODE_POINT: 只用点描边多边形区域
        // 如果需要使用非FILL模式，需要：
        //      VkPhysicalDeviceFeatures deviceFeatures{};
        //      deviceFeatures.fillModeNonSolid = VK_TRUE; // 启用线框/点模式
        rasterizerInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizerInfo.lineWidth = 1.0f; // 线段宽度，只有在polygonMode为LINE时才有意义
        // 背面剔除：丢弃背对观察者的三角形。对于单面渲染来说，背面通常不可见，所以可以启用背面剔除来提高性能。对于双面渲染来说，前后都要渲染，所以设置为VK_NONE。
        rasterizerInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        // 指定哪个面是正面。默认情况下，顶点按逆时针顺序定义的三角形被认为是正面。这里设置为顺时针，所以按顺时针顺序定义的三角形被认为是正面。
        rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizerInfo.depthBiasEnable = VK_FALSE;     // 深度偏移，通常用于阴影贴图。当前不需要，所以设置为VK_FALSE。
        rasterizerInfo.depthBiasConstantFactor = 0.0f; // 可选
        rasterizerInfo.depthBiasClamp = 0.0f;          // 可选
        rasterizerInfo.depthBiasSlopeFactor = 0.0f;    // 可选

        // Multisampling 多重采样
        // 将多个光栅化器采样点合并成一个片段的颜色。启用多重采样可以减少锯齿，但会增加性能开销。当前不需要，所以设置为VK_FALSE。
        VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
        multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisamplingInfo.sampleShadingEnable = VK_FALSE;
        multisamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisamplingInfo.minSampleShading = 1.0f;          // Optional
        multisamplingInfo.pSampleMask = nullptr;            // Optional
        multisamplingInfo.alphaToCoverageEnable = VK_FALSE; // Optional
        multisamplingInfo.alphaToOneEnable = VK_FALSE;      // Optional

        // Depth and stencil testing 深度和模板测试

        // Color blending 颜色混合
        // 片元着色器返回颜色后，需要将其与framebuffer中已有颜色混色。有两种方式
        // 1. 将旧值与新值混色得到最终颜色
        // 2. 使用按位运算合并旧值和新值

        // 每个附加帧缓冲区的配置
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        // 决定最终哪些通道会被写入framebuffer。现在全开了，所有通道都会写入
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;                     // 不启用混合，直接覆盖旧值
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional

        // 全局颜色混合设置
        VkPipelineColorBlendStateCreateInfo colorBlendingInfo{};
        colorBlendingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendingInfo.logicOpEnable = VK_FALSE;   // 不启用按位运算合并，直接使用混合结果
        colorBlendingInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
        colorBlendingInfo.attachmentCount = 1;
        colorBlendingInfo.pAttachments = &colorBlendAttachment;
        colorBlendingInfo.blendConstants[0] = 0.0f; //
        colorBlendingInfo.blendConstants[1] = 0.0f; // Optional
        colorBlendingInfo.blendConstants[2] = 0.0f; // Optional
        colorBlendingInfo.blendConstants[3] = 0.0f; // Optional

        // Pipeline layout 管线布局
        // 告诉pipeline，shader会从外部接收哪些数据。
        // 比如vertex shader需要MVP，fragment shader需要纹理采样器读贴图。它们是通过uniform/desciptor set传入的。
        // 管线布局描述了这些uniform和descriptor set的接口: 有几组数据、每组里面有什么、绑定在哪个位置

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;            // Optional
        pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        // ------------------------------------------------------------------------------
        // 清理阶段，销毁着色器模块
        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
    void MainLoop()
    {
        while (!glfwWindowShouldClose(window))
        {
            glfwPollEvents();
        }
    }

    void Cleanup()
    {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyRenderPass(device, renderPass, nullptr);
        for (auto imageView : swapChainImageViews)
        {
            vkDestroyImageView(device, imageView, nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
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

    void CreateInstance()
    {
        if (enableValidationLayers && !checkValidationLayerSupport())
        {
            throw std::runtime_error("validation layers requested, but not available!");
        }
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pEngineName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        std::vector<const char *> extensions = getRequiredExtensions();
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        // createInfo.enabledLayerCount = 0;

        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
        }
        else
        {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create instance");
        }
        // 总结：vk中创建Instance的步骤：
        // 1. creation info 指针
        // 2. allocator callbacks，通常为nullptr
        // 3. 存储新对象handle的变量的指针
    }

    bool checkValidationLayerSupport()
    {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char *layerName : validationLayers)
        {
            bool layerFound = false;
            for (const auto &layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0)
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
            {
                return false;
            }
        }
        return true;
    }

    std::vector<const char *> getRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // To understand extensions: e.g. on Windows, the following extensions are required:
        // glfwExtensions[0] -> "VK_KHR_surface"
        // glfwExtensions[1] -> "VK_KHR_win32_surface"
        std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
        return VK_FALSE;
    }

    GLFWwindow *window;
    VkInstance instance;

    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    VkDevice device;

    VkQueue graphicsQueue;

    VkSurfaceKHR surface;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    std::vector<VkImageView> swapChainImageViews;

    VkRenderPass renderPass;

    VkPipelineLayout pipelineLayout;
};

int main()
{
    HelloTriangleApplication app;
    try
    {
        app.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}