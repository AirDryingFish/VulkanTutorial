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
    AllocatedBuffer allocatedBuffer{};

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
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
    immediateSubmit([&](VkCommandBuffer commandBuffer) {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    });
}

void TriangleApplication::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    AllocatedBuffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    indexBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    mainDeletionQueue.pushFunction([this, buffer = indexBuffer]() mutable {
        destroyBuffer(indexBuffer);
    });

    copyBuffer(stagingBuffer.buffer, indexBuffer.buffer, bufferSize);
    destroyBuffer(stagingBuffer);
}

void TriangleApplication::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    AllocatedBuffer stagingBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    vertexBuffer = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    mainDeletionQueue.pushFunction([this, buffer = vertexBuffer]() mutable {
        destroyBuffer(vertexBuffer);
    });

    copyBuffer(stagingBuffer.buffer, vertexBuffer.buffer, bufferSize);
    destroyBuffer(stagingBuffer);
}

void TriangleApplication::createUniformBuffer()
{
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    uniformBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    {
        uniformBuffers[i] = createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vmaMapMemory(allocator, uniformBuffers[i].allocation, &uniformBufferMapped[i]);
        mainDeletionQueue.pushFunction([this, i]() {
            vmaUnmapMemory(allocator, uniformBuffers[i].allocation);
            destroyBuffer(uniformBuffers[i]);
        });
    }
}

void TriangleApplication::updateUniformBuffer(uint32_t currentImage)
{
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f);
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
