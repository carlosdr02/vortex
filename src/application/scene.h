#pragma once

#include <filesystem>

#include <vulkan/vulkan.h>
#include <graphics.h>

class Scene {
public:
    Scene() = default;
    Scene(Device* device);

    void add(const std::filesystem::path& path);

private:
    Device* device;
    Buffer vertexBuffer;
    Buffer indexBuffer;
    Buffer transformBuffer;
    Buffer blasBuffer;
    VkAccelerationStructureKHR blas;
};
