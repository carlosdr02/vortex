#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>
#include <graphics.h>

class Scene {
public:
    Scene() = default;
    Scene(Device* device);
    void destroy(VkDevice device);

    void add(const std::filesystem::path& path);

private:
    Device* device;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Buffer transformBuffer;
    Buffer blasBuffer;
    VkAccelerationStructureKHR blas;
    VkDeviceAddress blasDeviceAddress;
    Buffer instanceBuffer;
    Buffer tlasBuffer;
    VkAccelerationStructureKHR tlas;
    VkDeviceAddress tlasDeviceAddress;
};
