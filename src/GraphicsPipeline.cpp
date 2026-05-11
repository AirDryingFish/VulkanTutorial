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
void TriangleApplication::createRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat; // swap chain图像的格式
    // colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // 不使用多重采样
    colorAttachment.samples = msaaSamples;

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
    // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // 这张 image 即将被呈现到屏幕
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 这张 image 还要继续被当作颜色附件使用

    // 一个 render pass 里可以有多个 subpass，每个 subpass 是一个渲染步骤，后面的 subpass 可以读取前面 subpass 的输出结果。
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;                                    // 指定我们要使用第一个attachment
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // subpass使用attachment时的布局

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    // depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // pColorAttachments是关键部分。传入的是一个数组，数组的索引直接对应fragment shader里的layout(location = 0) out vec4 outColor;中的location。
    // 比如如果传入两个attachment，索引分别是0和1，那么fragment shader里就可以有两个输出变量，分别是layout(location = 0) out vec4 outColor0;和layout(location = 1) out vec4 outColor1;。
    // 如果fragment shader里只有一个输出变量，直接写layout(location = 0) out vec4 outColor;就行了，vulkan会自动把它绑定到索引为0的attachment上。
    // 这就是Multiple Render Targets (MRT)技术，可以在一个subpass里同时输出到多张图像上，性能更好。
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    // Render pass开始时会做布局转换(比如把图像从“未定义”转成“可以写颜色”的布局)
    // vulkan默认在管线最开头做这个转换，但是那时候图像可能还没从swap chain中那到。往一个没准备好的图像上做布局转换，会出问题
    // 解决思路：告诉vulkan，布局转换等到真正要写颜色的阶段再做
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // 外部，renderpass之间的操作
    dependency.dstSubpass = 0;                   // 我们的子通道

    /*
        顶点输入 (VERTEX_INPUT)
            ↓
        顶点着色 (VERTEX_SHADER)
            ↓
        几何着色 (GEOMETRY_SHADER)
            ↓
        光栅化早期 (EARLY_FRAGMENT_TESTS)  ← 早期深度测试
            ↓
        片段着色 (FRAGMENT_SHADER)
            ↓
        光栅化晚期 (LATE_FRAGMENT_TESTS)
            ↓
        颜色附件输出 (COLOR_ATTACHMENT_OUTPUT)  ← fragment 写到 framebuffer
            ↓
        传输 (TRANSFER)  ← 单独的阶段, vkCmdCopyImage 等在这运行
            ↓
        计算着色 (COMPUTE_SHADER)
    */

    // src 在 <某stage做的某种access> 完成， dst端的 <某stage做的某种access> 才能开始
    // => "上一次的 color/depth 写入(包括 LATE 测试阶段)必须真正完成、cache 都 flush 干净,这一次的 color/depth 写入(从 EARLY 测试阶段开始)才能动手。"
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create render pass!");
    }
    mainDeletionQueue.pushFunction([this, capturedRenderPass = renderPass]() mutable{
        vkDestroyRenderPass(device, capturedRenderPass, nullptr);
    });
}

std::vector<char> TriangleApplication::readFile(const std::string &filename)
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

VkShaderModule TriangleApplication::createShaderModule(const std::vector<char> &code)
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

void TriangleApplication::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    // binding 0上只有1个descriptor
    uboLayoutBinding.descriptorCount = 1;
    // 只在顶点着色器中使用
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
    mainDeletionQueue.pushFunction([this, layout = descriptorSetLayout]() mutable {
        vkDestroyDescriptorSetLayout(device, layout, nullptr);
    });
}

void TriangleApplication::createGraphicsPipeline()
{
    auto vertShaderCode = readFile("../shaders/vert.spv");
    auto fragShaderCode = readFile("../shaders/frag.spv");

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

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescriptions();

    // 顶点输入
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // 数据怎么从buffer里读：每个顶点占多少字节、是逐顶点读还是逐实例读。“一行数据有多宽”
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    // 顶点里有哪些属性、怎么拆分
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

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
    rasterizerInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
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
    // rasterizerInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizerInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerInfo.depthBiasEnable = VK_FALSE;     // 深度偏移，通常用于阴影贴图。当前不需要，所以设置为VK_FALSE。
    rasterizerInfo.depthBiasConstantFactor = 0.0f; // 可选
    rasterizerInfo.depthBiasClamp = 0.0f;          // 可选
    rasterizerInfo.depthBiasSlopeFactor = 0.0f;    // 可选

    // Multisampling 多重采样
    // 将多个光栅化器采样点合并成一个片段的颜色。启用多重采样可以减少锯齿，但会增加性能开销。当前不需要，所以设置为VK_FALSE。
    VkPipelineMultisampleStateCreateInfo multisamplingInfo{};
    multisamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingInfo.sampleShadingEnable = VK_FALSE;
    multisamplingInfo.rasterizationSamples = msaaSamples;
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
    pipelineLayoutInfo.setLayoutCount = 1;                 // Optional
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;         // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;      // Optional
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    mainDeletionQueue.pushFunction([this, layout = pipelineLayout]() mutable {
        vkDestroyPipelineLayout(device, layout, nullptr);
    });


    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS; // 新 fragment 的深度更小，更小的 <保留>

    // 用于保留落在指定深度范围内的片段。不启用此功能
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f;
    depthStencil.maxDepthBounds = 1.0f;

    // 用于配置模板缓冲区操作
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {};
    depthStencil.back = {};

    // 最终的管线的创建
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // 两个着色阶段：顶点着色器和片段着色器
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
    pipelineInfo.pViewportState = &viewportStateInfo;
    pipelineInfo.pRasterizationState = &rasterizerInfo;
    pipelineInfo.pMultisampleState = &multisamplingInfo;
    pipelineInfo.pDepthStencilState = nullptr; // Optional
    pipelineInfo.pColorBlendState = &colorBlendingInfo;
    pipelineInfo.pDynamicState = &dynamicStatesInfo; // Optional

    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0; // 指定这个管线将在哪个subpass里使用。当前只有一个subpass，所以直接设置为0

    // 如果要创建的新管线和某个已有管线大部分配置都相同，Vulkan允许从已有管线派生新管线，驱动可能会因此优化创建速度和运行时切换开销
    // * basePipelineHandle: 传入一个已经创建好的管线对象的handle，表示“要基于这条管线派生”
    // * basePipelineIndex: 当一次性批量创建多条管线时(vkCreateGraphicsPipelines支持传入数组)，可以用数组下标引用同批次中的另一条管线作为父管线
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // No other pipelines to derive from
    pipelineInfo.basePipelineIndex = -1;

    pipelineInfo.pDepthStencilState = &depthStencil;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    mainDeletionQueue.pushFunction([this, pipeline = graphicsPipeline]() mutable {
        vkDestroyPipeline(device, pipeline, nullptr);
    });

    // ------------------------------------------------------------------------------
    // 清理阶段，销毁着色器模块
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void TriangleApplication::createFramebuffers()
{
    // 1 个swap chain图像对应1个framebuffer，所以framebuffer的数量和swap chain图像的数量一样多
    swapChainFramebuffers.resize(swapChainImageViews.size());
    // 遍历 ImageView，给每个ImageView创建一个对应的framebuffer
    for (size_t i = 0; i < swapChainImageViews.size(); i++)
    {

        // std::array<VkImageView, 3> attachments = {
        //     swapChainImageViews[i],
        //     depthImageView,
        //     colorImageView};
        std::array<VkImageView, 3> attachments = {
            colorImage.imageView,
            depthImage.imageView,
            swapChainImageViews[i]};
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass; // framebuffer要兼容哪个render pass
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data(); // framebuffer要绑定哪些图像作为附件
        framebufferInfo.width = swapChainExtent.width;     // framebuffer的宽高必须和render pass里定义的视口大小一致
        framebufferInfo.height = swapChainExtent.height;   // framebuffer的宽高必须和render pass里定义的视口大小一致
        framebufferInfo.layers = 1;                        // 只有一层

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create framebuffer!");
        }

        swapChainDeletionQueue.pushFunction([this, frameBuffer = swapChainFramebuffers[i]]() {
            vkDestroyFramebuffer(device, frameBuffer, nullptr);
        });
    }
}
