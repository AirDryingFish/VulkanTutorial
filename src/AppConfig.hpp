#pragma once

#include "VulkanHeaders.hpp"

#include <cstdint>
#include <string>
#include <vector>

inline constexpr uint32_t WIDTH = 1920;
inline constexpr uint32_t HEIGHT = 1080;

inline const std::string MODEL_PATH = "../models/viking_room.obj";
inline const std::string TEXTURE_PATH = "../textures/viking_room.png";
inline const std::string PBR_ALBEDO_PATH = "../textures/pbr/rustediron2_basecolor.png";
inline const std::string PBR_NORMAL_PATH = "../textures/pbr/rustediron2_normal.png";
inline const std::string PBR_METALLIC_PATH = "../textures/pbr/rustediron2_metallic.png";
inline const std::string PBR_ROUGHNESS_PATH = "../textures/pbr/rustediron2_roughness.png";
inline const std::string PBR_AO_PATH = "../textures/pbr/rustediron2_ao.png";

inline constexpr int MAX_FRAMES_IN_FLIGHT = 2;

inline const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

inline const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef NDEBUG
inline constexpr bool enableValidationLayers = false;
#else
inline constexpr bool enableValidationLayers = true;
#endif
