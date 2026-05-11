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
AllocatedBuffer TriangleApplication::createBuffer(VkDeviceSize size,
                                                  VkBufferUsageFlags usage,
                                                  VkMemoryPropertyFlags properties)
{

    // VkBufferCreateInfo bufferInfo{};
    // bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    // bufferInfo.size = size;
    // bufferInfo.usage = usage; // 这个buffer是用来当顶点缓冲区的
    // // 缓冲区可以由特定队列族所有，或者在多个队列族之间共享。
    // // 顶点缓冲区只会从图形队列中使用，因此可以使用EXCLUSIVE模式。只有当不同队列族需要访问同一个缓冲区时，才需要使用CONCURRENT模式，并且还需要指定所有访问该缓冲区的队列族索引。
    // bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    // if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("failed to create vertex buffer!");
    // }

    // // 分配内存
    // // step 1: 查询缓冲区需要什么样的内存。
    // // memRequirements告诉三件事：
    // // 1. size: 需要分配的内存大小(字节数)。这个值可能会根据buffer的usage和format等属性而变化。
    // // 2. alignment: 内存的对齐要求。分配的内存地址必须是alignment的倍数。这个值可能会根据buffer的usage和format等属性而变化。
    // // 3. memoryTypeBits: 一个位掩码，表示哪些类型的内存可以用来分配这个buffer。每个位对应一个内存类型，如果某个位为1，表示该内存类型支持这个buffer的需求。这个值取决于buffer的usage和format等属性，以及物理设备的内存特性。
    // VkMemoryRequirements memRequirements;
    // vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    // VkMemoryAllocateInfo allocInfo{};
    // allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    // allocInfo.allocationSize = memRequirements.size;
    // // step 2: 根据第一步查询到的内存需求，找到一个既满足buffer需求又具有指定属性的内存类型。比如我们需要一个既能当作顶点缓冲区又可以直接从CPU访问的内存类型。
    // // HOST_VISIBLE + HOST_COHERENT: CPU可写，且自动同步
    // allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    // if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
    // {
    //     throw std::runtime_error("failed to allocate vertex buffer memory!");
    // }
    // vkBindBufferMemory(device, buffer, bufferMemory, 0);

    AllocatedBuffer allocatedBuffer{};

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage; // 这个buffer是用来当顶点缓冲区的
    // 缓冲区可以由特定队列族所有，或者在多个队列族之间共享。
    // 顶点缓冲区只会从图形队列中使用，因此可以使用EXCLUSIVE模式。只有当不同队列族需要访问同一个缓冲区时，才需要使用CONCURRENT模式，并且还需要指定所有访问该缓冲区的队列族索引。
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    if (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
    {
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    }
    else
    {
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
    }
    if (vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &allocatedBuffer.buffer, &allocatedBuffer.allocation, nullptr) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create buffer!");
    }
    return allocatedBuffer;
}

void TriangleApplication::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
    // VkCommandBuffer commandBuffer = beginSingleTimeCommands();
    immediateSubmit([&](VkCommandBuffer commandBuffer){
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0; // Optional
        copyRegion.dstOffset = 0; // Optional
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
    // endSingleTimeCommands(commandBuffer);
}

void TriangleApplication::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    // VkBuffer stagingBuffer;
    // // VkDeviceMemory stagingBuffferMemory;
    // VmaAllocation stagingBufferAllocation;
    AllocatedBuffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    // vkMapMemory(device, stagingBufferAllocation, 0, bufferSize, 0, &data);
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    // vkUnmapMemory(device, stagingBuffferMemory);`
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    indexBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    mainDeletionQueue.pushFunction([this, buffer = indexBuffer]() mutable{
        destroyBuffer(indexBuffer);
    });

    copyBuffer(stagingBuffer.buffer, indexBuffer.buffer, bufferSize);

    // vkDestroyBuffer(device, stagingBuffer, nullptr);
    // vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    destroyBuffer(stagingBuffer);
}

void TriangleApplication::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    // VkBuffer stagingBuffer;
    // // VkDeviceMemory stagingBufferMemory;
    // VmaAllocation stagingBufferAllocation;
    AllocatedBuffer stagingBuffer;
    stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    vertexBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    mainDeletionQueue.pushFunction([this, buffer = vertexBuffer]() mutable {
        destroyBuffer(vertexBuffer);
    });

    copyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);
    // vkDestroyBuffer(device, stagingBuffer, nullptr);
    // vkFreeMemory(device, stagingBufferMemory, nullptr);
    // vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    destroyBuffer(stagingBuffer);
}

void TriangleApplication::createUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    // uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        uniformBuffers[i] = createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        // vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBufferMapped[i]);
        vmaMapMemory(allocator, uniformBuffers[i].allocation, &uniformBufferMapped[i]);
        mainDeletionQueue.pushFunction([this, i](){
            vmaUnmapMemory(allocator, uniformBuffers[i].allocation);
            destroyBuffer(uniformBuffers[i]);
        });
    }
}

// uint32_t TriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
// {
//     VkPhysicalDeviceMemoryProperties memProperties;
//     vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
//     for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
//     {
//         // typeFilter & (1 << i): 检查第i种内存类型是否在缓冲区支持的类型里（由第一步的memoryTypeBits决定）
//         // memProperties.memoryTypes[i].propertyFlags & properties: 检查第i种内存类型是否具有我们需要的属性（比如HOST_VISIBLE和HOST_COHERENT）
//         if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
//         {
//             return i;
//         }
//     }
//     throw std::runtime_error("failed to find suitable memory type!");
// }

void TriangleApplication::updateUniformBuffer(uint32_t currentImage)
{
    // static只会在第一次调用时初始化一次
    static auto startTime = std::chrono::high_resolution_clock::now();
    // 当前时间
    auto currentTime = std::chrono::high_resolution_clock::now();
    // 获取运行了多长时间
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    // glm::mat4(1.0f) 返回单位矩阵；time * glm::radians(90.0f) 每秒旋转90°； glm::vec3(0.0f, 0.0f, 1.0f)绕 z轴 旋转
    // ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.model = glm::mat4(1.0f);

    // 摄像机在 (2,2,2) 位置看向原点 (0,0,0)，相当于从斜上方 45 度俯视
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    // 透视投影，45 度垂直视场角，宽高比用 swap chain 当前尺寸算
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 10.0f);
    ubo.proj[1][1] *= -1;
    memcpy(uniformBufferMapped[currentFrame], &ubo, sizeof(ubo));
}

void TriangleApplication::destroyBuffer(AllocatedBuffer &buffer)
{
    if (buffer.buffer != VK_NULL_HANDLE)
    {
        vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
        buffer.buffer = VK_NULL_HANDLE;
        buffer.allocation = nullptr;
    }
}