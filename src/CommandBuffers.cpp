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
void TriangleApplication::createCommandPool()
{
    QueueFamilyIndices QueueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    // 命令池有两种可能的flag:
    // * VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: 命令缓冲区是短命的，可能会频繁重置和录制。驱动可以为这种命令缓冲区优化内存分配。
    // * VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: 允许单独重置命令池中的每个命令缓冲区，而不是必须重置整个命令池。这对于需要频繁重置命令缓冲区的应用程序很有用。如果没有此标志，所有命令缓冲区必须一起重置，可能会导致性能下降。
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = QueueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create command pool!");
    }
}

void TriangleApplication::createCommandBuffers()
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    // level 参数指定分配的command buffer是主命令缓冲区还是次级命令缓冲区。主命令缓冲区可以直接提交到队列执行，次级命令缓冲区不能直接提交，必须被主命令缓冲区调用才能执行。
    // 次级命令缓冲区通常用于多线程录制命令，或者当你想要重用一部分命令时。当前我们只需要一个主命令缓冲区，所以设置为VK_COMMAND_BUFFER_LEVEL_PRIMARY。
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void TriangleApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // flags 参数指定如何使用 command buffer
    // * VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: command buffer 只会被提交执行一次。使用完后就会被重置或销毁。驱动可以为这种命令缓冲区优化内存分配。
    // * VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: command buffer 将在一个 render pass 中被执行。这个 flag 只能用于次级命令缓冲区，主命令缓冲区不能设置这个 flag。
    // * VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: command buffer 可以同时被多个队列提交执行。默认情况下，command buffer 在一次提交执行后就不能再次提交，必须等到执行完成后才能再次提交。如果设置了这个 flag，就可以在 command buffer 执行时再次提交它，前提是它之前的执行已经完成了。
    beginInfo.flags = 0; // Optional
    // pInheritanceInfo 只有当command buffer是次级命令缓冲区时才需要这个参数，它告诉vulkan次级命令缓冲区将被哪个主命令缓冲区调用，以及在什么render pass和subpass中被调用。当前我们只使用主命令缓冲区，所以设置为nullptr。
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // start a render pass
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];

    // 定义渲染区域的大小。渲染区域决定了shader加载和存储数据的位置。该区域外的像素将有未定义的值。渲染区域的大小应该与attachment的大小相匹配，这样才能正确渲染到整个图像上。当前我们设置渲染区域覆盖整个swap chain图像。
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    // VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}}; // 定义clear color，清空颜色缓冲区时会用到
    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    // Param 1: 记录命令的Command Buffer
    // Param 2: render pass的创建信息，告诉vulkan我们要使用哪个render pass、哪个framebuffer、渲染区域的大小、clear color等
    // Param 3:
    // * VK_SUBPASS_CONTENTS_INLINE: 命令直接记录在主命令缓冲区里，不使用次级命令缓冲区。这是最简单的方式，适合大多数情况。
    // * VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: 渲染通道命令将从次级命令缓冲区中获取
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline); // 绑定管线对象，告诉vulkan后续的绘制命令要使用哪个管线

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = {vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets); // 绑定顶点缓冲区，告诉vulkan后续的绘制命令要从哪个缓冲区读取顶点数据，以及每个顶点数据的偏移量

    vkCmdBindIndexBuffer(commandBuffer, indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32); // 绑定索引缓冲区，告诉vulkan后续的绘制命令要从哪个缓冲区读取索引数据，以及每个索引数据的偏移量和类型

    // vertexCount = 3: 画3个顶点
    // instanceCount = 1: 画1份实例。如果设置为100，同样的3个顶点会画100次，每次gl_InstanceIndex不同，常用于草地、树木等大量重复物体的高效渲染
    // fistVertex = 0: gl_VertexIndex从0开始。如果设置为5，那么就从5开始，相当于跳过前5个顶点
    // firstInstance = 0: gl_InstanceIndex从0开始。如果设置为5，那么就从5开始，相当于跳过前5份实例
    // 画1个实例，每个实例3个顶点，从头开始画
    // vkCmdDraw(commandBuffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr); // 绑定描述符集，告诉vulkan后续的绘制命令要使用哪个描述符集来获取shader需要的外部数据（比如uniform buffer）

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0); // 画1个实例，每个实例3个顶点，从头开始画

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to record command buffer!");
    }
}

VkCommandBuffer TriangleApplication::beginSingleTimeCommands()
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    return commandBuffer;
}

void TriangleApplication::endSingleTimeCommands(VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}
