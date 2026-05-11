#include "TriangleApplication.hpp"

#include <stb_image.h>

#include <array>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
constexpr std::array<const char *, 6> skyboxFaces = {
    "../textures/skybox/right.jpg",
    "../textures/skybox/left.jpg",
    "../textures/skybox/top.jpg",
    "../textures/skybox/bottom.jpg",
    "../textures/skybox/front.jpg",
    "../textures/skybox/back.jpg",
};

constexpr std::array<std::array<unsigned char, 4>, 6> fallbackSkyboxColors = {
    std::array<unsigned char, 4>{70, 110, 180, 255},
    std::array<unsigned char, 4>{65, 100, 165, 255},
    std::array<unsigned char, 4>{115, 155, 215, 255},
    std::array<unsigned char, 4>{35, 45, 65, 255},
    std::array<unsigned char, 4>{80, 125, 190, 255},
    std::array<unsigned char, 4>{50, 80, 140, 255},
};
}

void TriangleApplication::createSkyboxImage()
{
    int width = 0;
    int height = 0;
    std::array<stbi_uc *, 6> loadedPixels{};
    bool useFallback = false;

    for (size_t i = 0; i < skyboxFaces.size(); i++)
    {
        int faceWidth = 0;
        int faceHeight = 0;
        int channels = 0;
        loadedPixels[i] = stbi_load(skyboxFaces[i], &faceWidth, &faceHeight, &channels, STBI_rgb_alpha);
        if (loadedPixels[i] == nullptr)
        {
            useFallback = true;
            break;
        }
        if (faceWidth != faceHeight)
        {
            useFallback = true;
            break;
        }

        if (i == 0)
        {
            width = faceWidth;
            height = faceHeight;
        }
        else if (faceWidth != width || faceHeight != height)
        {
            useFallback = true;
            break;
        }
    }

    if (useFallback)
    {
        for (stbi_uc *pixels : loadedPixels)
        {
            stbi_image_free(pixels);
        }
        width = 1;
        height = 1;
    }

    const VkDeviceSize layerSize = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;
    const VkDeviceSize imageSize = layerSize * 6;

    AllocatedBuffer stagingBuffer = createBuffer(
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data = nullptr;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    for (size_t i = 0; i < skyboxFaces.size(); i++)
    {
        unsigned char *dst = static_cast<unsigned char *>(data) + layerSize * i;
        if (useFallback)
        {
            std::memcpy(dst, fallbackSkyboxColors[i].data(), fallbackSkyboxColors[i].size());
        }
        else
        {
            std::memcpy(dst, loadedPixels[i], static_cast<size_t>(layerSize));
            stbi_image_free(loadedPixels[i]);
        }
    }
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    skyboxImage = createImage(
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height),
        1,
        VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        6,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

    transitionImageLayout(
        skyboxImage.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        6);

    immediateSubmit([&](VkCommandBuffer commandBuffer) {
        std::array<VkBufferImageCopy, 6> regions{};
        for (uint32_t i = 0; i < regions.size(); i++)
        {
            regions[i].bufferOffset = layerSize * i;
            regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[i].imageSubresource.mipLevel = 0;
            regions[i].imageSubresource.baseArrayLayer = i;
            regions[i].imageSubresource.layerCount = 1;
            regions[i].imageExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
                1};
        }

        vkCmdCopyBufferToImage(
            commandBuffer,
            stagingBuffer.buffer,
            skyboxImage.image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            static_cast<uint32_t>(regions.size()),
            regions.data());
    });

    transitionImageLayout(
        skyboxImage.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        1,
        6);

    skyboxImage.imageView = createImageView(
        skyboxImage.image,
        VK_FORMAT_R8G8B8A8_SRGB,
        1,
        VK_IMAGE_ASPECT_COLOR_BIT,
        VK_IMAGE_VIEW_TYPE_CUBE,
        6);

    destroyBuffer(stagingBuffer);

    mainDeletionQueue.pushFunction([this, image = skyboxImage]() mutable {
        destroyImage(image);
    });
}

void TriangleApplication::createSkyboxSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &skyboxSampler) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create skybox sampler!");
    }

    mainDeletionQueue.pushFunction([this, sampler = skyboxSampler]() {
        vkDestroySampler(device, sampler, nullptr);
    });
}
