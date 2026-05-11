#include "TriangleApplication.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

namespace
{
ImVec2 operator+(const ImVec2 &a, const ImVec2 &b)
{
    return ImVec2(a.x + b.x, a.y + b.y);
}

ImVec2 operator-(const ImVec2 &a, const ImVec2 &b)
{
    return ImVec2(a.x - b.x, a.y - b.y);
}

ImVec2 operator*(const ImVec2 &v, float scale)
{
    return ImVec2(v.x * scale, v.y * scale);
}

float dot(const ImVec2 &a, const ImVec2 &b)
{
    return a.x * b.x + a.y * b.y;
}

float length(const ImVec2 &v)
{
    return std::sqrt(dot(v, v));
}

ImVec2 normalize(const ImVec2 &v)
{
    const float len = length(v);
    if (len < 0.0001f)
    {
        return ImVec2(0.0f, 0.0f);
    }
    return v * (1.0f / len);
}

float distancePointToSegment(const ImVec2 &point, const ImVec2 &a, const ImVec2 &b)
{
    const ImVec2 ab = b - a;
    const float abLengthSq = dot(ab, ab);
    if (abLengthSq < 0.0001f)
    {
        return length(point - a);
    }

    const float t = std::clamp(dot(point - a, ab) / abLengthSq, 0.0f, 1.0f);
    const ImVec2 closest = a + ab * t;
    return length(point - closest);
}

bool worldToScreen(const glm::vec3 &worldPoint, const glm::mat4 &view, const glm::mat4 &projection, const ImGuiViewport *viewport, ImVec2 &screenPoint)
{
    glm::vec4 clip = projection * view * glm::vec4(worldPoint, 1.0f);
    if (clip.w <= 0.0001f)
    {
        return false;
    }

    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    if (ndc.z < 0.0f || ndc.z > 1.0f)
    {
        return false;
    }

    screenPoint.x = viewport->Pos.x + (ndc.x * 0.5f + 0.5f) * viewport->Size.x;
    screenPoint.y = viewport->Pos.y + (ndc.y * 0.5f + 0.5f) * viewport->Size.y;
    return true;
}

void buildMouseRay(const ImVec2 &mouse, const glm::mat4 &view, const glm::mat4 &projection, const ImGuiViewport *viewport, glm::vec3 &rayOrigin, glm::vec3 &rayDirection)
{
    const float ndcX = ((mouse.x - viewport->Pos.x) / viewport->Size.x) * 2.0f - 1.0f;
    const float ndcY = ((mouse.y - viewport->Pos.y) / viewport->Size.y) * 2.0f - 1.0f;

    const glm::mat4 inverseViewProjection = glm::inverse(projection * view);
    glm::vec4 nearPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
    glm::vec4 farPoint = inverseViewProjection * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;

    rayOrigin = glm::vec3(nearPoint);
    rayDirection = glm::normalize(glm::vec3(farPoint - nearPoint));
}

bool intersectRayPlane(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const glm::vec3 &planePoint, const glm::vec3 &planeNormal, glm::vec3 &hitPoint)
{
    const float denominator = glm::dot(rayDirection, planeNormal);
    if (std::abs(denominator) < 0.0001f)
    {
        return false;
    }

    const float distance = glm::dot(planePoint - rayOrigin, planeNormal) / denominator;
    if (distance < 0.0f)
    {
        return false;
    }

    hitPoint = rayOrigin + rayDirection * distance;
    return true;
}

glm::vec3 makeGizmoDragPlaneNormal(const glm::vec3 &axisDirection, const glm::vec3 &cameraFront, const glm::vec3 &cameraUp)
{
    glm::vec3 planeNormal = cameraFront - axisDirection * glm::dot(cameraFront, axisDirection);
    if (glm::length(planeNormal) > 0.0001f)
    {
        return glm::normalize(planeNormal);
    }

    planeNormal = cameraUp - axisDirection * glm::dot(cameraUp, axisDirection);
    if (glm::length(planeNormal) > 0.0001f)
    {
        return glm::normalize(planeNormal);
    }

    planeNormal = glm::cross(axisDirection, glm::vec3(1.0f, 0.0f, 0.0f));
    if (glm::length(planeNormal) > 0.0001f)
    {
        return glm::normalize(planeNormal);
    }

    return glm::normalize(glm::cross(axisDirection, glm::vec3(0.0f, 1.0f, 0.0f)));
}

void drawAxisLine(ImDrawList *drawList, const ImVec2 &origin, const glm::mat3 &viewRotation, const glm::vec3 &axis, ImU32 color, const char *label)
{
    glm::vec3 viewAxis = viewRotation * axis;
    ImVec2 direction(viewAxis.x, -viewAxis.y);
    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    if (length < 0.001f)
    {
        direction = ImVec2(0.0f, viewAxis.z < 0.0f ? -1.0f : 1.0f);
        length = 1.0f;
    }

    direction = direction * (1.0f / length);
    const float axisLength = 36.0f;
    const ImVec2 end = origin + direction * axisLength;
    const ImVec2 normal(-direction.y, direction.x);

    drawList->AddLine(origin, end, color, 3.0f);
    drawList->AddTriangleFilled(
        end,
        end - direction * 8.0f + normal * 4.0f,
        end - direction * 8.0f - normal * 4.0f,
        color);
    drawList->AddText(end + direction * 6.0f + ImVec2(-4.0f, -7.0f), color, label);
}

ImU32 axisColor(int axis, bool highlighted)
{
    if (axis == 1)
    {
        return highlighted ? IM_COL32(255, 95, 95, 255) : IM_COL32(230, 60, 60, 255);
    }
    if (axis == 2)
    {
        return highlighted ? IM_COL32(120, 255, 130, 255) : IM_COL32(80, 210, 90, 255);
    }
    return highlighted ? IM_COL32(120, 170, 255, 255) : IM_COL32(80, 140, 255, 255);
}

const char *axisLabel(int axis)
{
    if (axis == 1)
    {
        return "X";
    }
    if (axis == 2)
    {
        return "Y";
    }
    return "Z";
}
}

void TriangleApplication::initImGui()
{
    // 为什么Imgui需要descriptor？
    // descriptor是渲染的时候需要渲染UI，渲染一个Image或者Text的时候
    // 每张纹理用一个descriptor，Imgui内部维护VkImageView->VkDescriptorSet的映射
    VkDescriptorPoolSize poolSize{
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        1000};
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiDescriptorPool) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to create imgui descriptor pool!");
    }
    mainDeletionQueue.pushFunction([this, pool = imguiDescriptorPool]() mutable
                                   { vkDestroyDescriptorPool(device, pool, nullptr); });

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForVulkan(window, true);

    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.ApiVersion = VK_API_VERSION_1_0;
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = physicalDevice;
    initInfo.Device = device;
    initInfo.QueueFamily = indices.graphicsFamily.value();
    initInfo.Queue = graphicsQueue;
    initInfo.DescriptorPool = imguiDescriptorPool;
    initInfo.RenderPass = renderPass;
    initInfo.MinImageCount = MAX_FRAMES_IN_FLIGHT;
    initInfo.ImageCount = static_cast<uint32_t>(swapChainImages.size());
    initInfo.MSAASamples = msaaSamples;

    if (!ImGui_ImplVulkan_Init(&initInfo))
    {
        throw std::runtime_error("failed to init imgui vulkan backend!");
    }
    if (!ImGui_ImplVulkan_CreateFontsTexture())
    {
        throw std::runtime_error("failed to create imgui font texture!");
    }
    mainDeletionQueue.pushFunction([]()
                                   {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(); });
}

void TriangleApplication::drawImGui()
{
    ImGuiIO &io = ImGui::GetIO();

    ImGui::Text("FPS: %.1f", io.Framerate);
    ImGui::Text("Frame Time: %.3f ms", 1000.0f / io.Framerate);
    ImGui::Text("Swapchain Images: %zu", swapChainImages.size());
    ImGui::Text("Extent: %u x %u", swapChainExtent.width, swapChainExtent.height);
    ImGui::Text("MSAA Samples: %d", msaaSamples);

    ImGui::SeparatorText("Camera");
    ImGui::Text("Position: %.2f %.2f %.2f", cameraPos.x, cameraPos.y, cameraPos.z);
    ImGui::Text("Front: %.2f %.2f %.2f", cameraFront.x, cameraFront.y, cameraFront.z);
    ImGui::Text("Yaw/Pitch: %.1f %.1f", cameraYaw, cameraPitch);
    ImGui::DragFloat("Near Plane", &cameraNear, 0.01f, 0.01f, 10.0f);
    ImGui::DragFloat("Far Plane", &cameraFar, 1.0f, 10.0f, 10000.0f);
    ImGui::DragFloat("Move Speed", &cameraMoveSpeed, 0.1f, 0.1f, 30.0f);
    ImGui::DragFloat("Fast Multiplier", &cameraFastMultiplier, 0.1f, 1.0f, 10.0f);
    ImGui::DragFloat("Scroll Speed", &cameraScrollSpeed, 0.1f, 0.1f, 20.0f);
    ImGui::DragFloat("Pan Speed", &cameraPanSpeed, 0.001f, 0.001f, 0.2f, "%.3f");
    ImGui::DragFloat("Mouse Sensitivity", &mouseSensitivity, 0.01f, 0.01f, 2.0f);

    ImGui::SeparatorText("Model Transform");
    ImGui::Text("Selected: %s", selectedModel ? "true" : "false");
    if (selectedModel)
    {
        ImGui::Text("Pick Distance: %.3f", modelPickDistance);
    }
    ImGui::Checkbox("Auto Rotate", &rotateModel);
    ImGui::DragFloat("Auto Rotate Speed", &modelAutoRotateSpeed, 1.0f, -720.0f, 720.0f);
    ImGui::DragFloat3("Position", &modelPosition.x, 0.05f);
    ImGui::DragFloat3("Rotation", &modelRotation.x, 0.5f, -360.0f, 360.0f);
    ImGui::DragFloat3("Scale", &modelScale.x, 0.02f, 0.01f, 20.0f);
    if (ImGui::Button("Reset Transform"))
    {
        modelPosition = glm::vec3(0.0f);
        modelRotation = glm::vec3(0.0f);
        modelScale = glm::vec3(1.0f);
        modelAutoRotation = 0.0f;
    }

    ImGui::SeparatorText("Resources");

    ImGui::Text("Vertices: %zu", vertices.size());
    ImGui::Text("Indices: %zu", indices.size());
    ImGui::Text("AABB Min: %.2f %.2f %.2f", modelLocalBoundsMin.x, modelLocalBoundsMin.y, modelLocalBoundsMin.z);
    ImGui::Text("AABB Max: %.2f %.2f %.2f", modelLocalBoundsMax.x, modelLocalBoundsMax.y, modelLocalBoundsMax.z);
    ImGui::Text("Mip Levels: %u", mipLevels);
    ImGui::Text("Texture Image: 0x%p", (void *)textureImage.image);

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    const float radius = 50.0f;
    const float margin = 24.0f;
    const ImVec2 origin(
        viewport->Pos.x + viewport->Size.x - margin - radius,
        viewport->Pos.y + margin + radius);

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat3 viewRotation(view);

    drawList->AddCircleFilled(origin, radius, IM_COL32(16, 18, 22, 150), 32);
    drawList->AddCircle(origin, radius, IM_COL32(255, 255, 255, 45), 32, 1.0f);
    drawAxisLine(drawList, origin, viewRotation, glm::vec3(1.0f, 0.0f, 0.0f), IM_COL32(230, 60, 60, 255), "X");
    drawAxisLine(drawList, origin, viewRotation, glm::vec3(0.0f, 1.0f, 0.0f), IM_COL32(80, 210, 90, 255), "Y");
    drawAxisLine(drawList, origin, viewRotation, glm::vec3(0.0f, 0.0f, 1.0f), IM_COL32(80, 140, 255, 255), "Z");

    drawTransformGizmo();
}

void TriangleApplication::drawTransformGizmo()
{
    if (!selectedModel)
    {
        gizmoHoveredAxis = 0;
        gizmoActiveAxis = 0;
        return;
    }

    ImDrawList *drawList = ImGui::GetForegroundDrawList();
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiIO &io = ImGui::GetIO();

    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), cameraNear, cameraFar);
    projection[1][1] *= -1;

    glm::mat4 model = getModelMatrix();
    glm::vec3 originWorld = glm::vec3(model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
    glm::vec3 axisDirections[3] = {
        glm::normalize(glm::vec3(model * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f))),
        glm::normalize(glm::vec3(model * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f))),
        glm::normalize(glm::vec3(model * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f))),
    };

    const float distanceToCamera = glm::length(cameraPos - originWorld);
    const float axisWorldLength = std::clamp(distanceToCamera * 0.18f, 0.3f, 3.0f);

    ImVec2 originScreen;
    if (!worldToScreen(originWorld, view, projection, viewport, originScreen))
    {
        gizmoHoveredAxis = 0;
        return;
    }

    struct AxisScreenData
    {
        ImVec2 end;
        bool visible = false;
        bool interactive = false;
        float screenLength = 0.0f;
    };

    AxisScreenData axes[3];
    constexpr float minInteractiveAxisScreenLength = 24.0f;
    for (int i = 0; i < 3; i++)
    {
        const glm::vec3 axisEndWorld = originWorld + axisDirections[i] * axisWorldLength;
        axes[i].visible = worldToScreen(axisEndWorld, view, projection, viewport, axes[i].end);
        axes[i].screenLength = axes[i].visible ? length(axes[i].end - originScreen) : 0.0f;
        axes[i].interactive = axes[i].screenLength >= minInteractiveAxisScreenLength;
    }

    const ImVec2 mouse = io.MousePos;
    gizmoHoveredAxis = 0;
    if (gizmoActiveAxis == 0 && !io.WantCaptureMouse)
    {
        float bestDistance = 12.0f;
        for (int i = 0; i < 3; i++)
        {
            if (!axes[i].visible || !axes[i].interactive)
            {
                continue;
            }

            const float distance = distancePointToSegment(mouse, originScreen, axes[i].end);
            if (distance < bestDistance)
            {
                bestDistance = distance;
                gizmoHoveredAxis = i + 1;
            }
        }
    }

    if (gizmoHoveredAxis != 0)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    if (gizmoActiveAxis == 0 && gizmoHoveredAxis != 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        const int axisIndex = gizmoHoveredAxis - 1;
        glm::vec3 rayOrigin;
        glm::vec3 rayDirection;
        glm::vec3 hitPoint;
        const glm::vec3 planeNormal = makeGizmoDragPlaneNormal(axisDirections[axisIndex], cameraFront, cameraUp);

        buildMouseRay(mouse, view, projection, viewport, rayOrigin, rayDirection);
        if (intersectRayPlane(rayOrigin, rayDirection, originWorld, planeNormal, hitPoint))
        {
            gizmoActiveAxis = gizmoHoveredAxis;
            gizmoDragStartMouse = glm::vec2(mouse.x, mouse.y);
            gizmoDragStartPosition = modelPosition;
            gizmoDragAxis = axisDirections[axisIndex];
            gizmoDragPlaneNormal = planeNormal;
            gizmoDragStartHitPoint = hitPoint;
        }
    }

    if (gizmoActiveAxis != 0)
    {
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            gizmoActiveAxis = 0;
        }
        else
        {
            const int axisIndex = gizmoActiveAxis - 1;
            glm::vec3 rayOrigin;
            glm::vec3 rayDirection;
            glm::vec3 hitPoint;

            buildMouseRay(mouse, view, projection, viewport, rayOrigin, rayDirection);
            if (intersectRayPlane(rayOrigin, rayDirection, gizmoDragStartHitPoint, gizmoDragPlaneNormal, hitPoint))
            {
                const float worldDistance = glm::dot(hitPoint - gizmoDragStartHitPoint, gizmoDragAxis);
                modelPosition = gizmoDragStartPosition + gizmoDragAxis * worldDistance;
            }
        }
    }

    drawList->AddCircleFilled(originScreen, 5.0f, IM_COL32(245, 245, 245, 255), 16);
    drawList->AddCircle(originScreen, 7.0f, IM_COL32(20, 20, 20, 180), 16, 1.5f);

    for (int i = 0; i < 3; i++)
    {
        if (!axes[i].visible)
        {
            continue;
        }

        const int axis = i + 1;
        const bool highlighted = axis == gizmoHoveredAxis || axis == gizmoActiveAxis;
        const ImU32 color = axisColor(axis, highlighted);
        const float thickness = highlighted ? 5.0f : 3.0f;
        const ImVec2 direction = normalize(axes[i].end - originScreen);
        const ImVec2 normal(-direction.y, direction.x);

        drawList->AddLine(originScreen, axes[i].end, color, thickness);
        drawList->AddTriangleFilled(
            axes[i].end,
            axes[i].end - direction * 12.0f + normal * 6.0f,
            axes[i].end - direction * 12.0f - normal * 6.0f,
            color);
        drawList->AddText(axes[i].end + direction * 8.0f + ImVec2(-4.0f, -7.0f), color, axisLabel(axis));
    }
}
