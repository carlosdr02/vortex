#include "scene.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

Scene::Scene(Device* device) : device(device) {
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = device->renderQueue.familyIndex
    };

    vkCreateCommandPool(device->logical, &commandPoolCreateInfo, nullptr, &commandPool);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext              = nullptr,
        .commandPool        = commandPool,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(device->logical, &commandBufferAllocateInfo, &commandBuffer);
}

void Scene::destroy(VkDevice device) {
    vkDestroyAccelerationStructure(device, tlas, nullptr);
    vkDestroyAccelerationStructure(device, blas, nullptr);

    tlasBuffer.destroy(device);
    instanceBuffer.destroy(device);
    blasBuffer.destroy(device);
    transformBuffer.destroy(device);
    indexBuffer.destroy(device);
    vertexBuffer.destroy(device);

    vkDestroyCommandPool(device, commandPool, nullptr);
}

static void extractMeshData(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    // Access the position attribute (commonly stored in "POSITION" semantic)
    const auto& positionAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
    const auto& positionBufferView = model.bufferViews[positionAccessor.bufferView];
    const auto& positionBuffer = model.buffers[positionBufferView.buffer];

    // Get the pointer to the buffer data and its size
    const unsigned char* positionData = positionBuffer.data.data() + positionBufferView.byteOffset + positionAccessor.byteOffset;
    const size_t vertexCount = positionAccessor.count;
    const size_t vertexStride = positionAccessor.ByteStride(positionBufferView);

    // Store vertices
    vertices.resize(vertexCount * 3);  // assuming 3 floats per vertex (X, Y, Z)
    for (size_t i = 0; i < vertexCount; ++i) {
        memcpy(&vertices[i * 3], positionData + i * vertexStride, sizeof(float) * 3);  // copy X, Y, Z
    }

    // Access indices if available
    if (primitive.indices >= 0) {
        const auto& indexAccessor = model.accessors[primitive.indices];
        const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = model.buffers[indexBufferView.buffer];

        const unsigned char* indexData = indexBuffer.data.data() + indexBufferView.byteOffset + indexAccessor.byteOffset;
        const size_t indexCount = indexAccessor.count;

        // Store indices (consider different index formats: unsigned short, unsigned int, etc.)
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            indices.resize(indexCount);
            for (size_t i = 0; i < indexCount; ++i) {
                indices[i] = reinterpret_cast<const unsigned short*>(indexData)[i];
            }
        } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            indices.resize(indexCount);
            for (size_t i = 0; i < indexCount; ++i) {
                indices[i] = reinterpret_cast<const unsigned int*>(indexData)[i];
            }
        }
    }
}

void Scene::add(const std::filesystem::path& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.string());

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return;
    }

    const tinygltf::Mesh& mesh = model.meshes[0];
    const tinygltf::Primitive& primitive = mesh.primitives[0];

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    extractMeshData(model, primitive, vertices, indices);

    const VkDeviceSize vertexBufferSize = vertices.size() * sizeof(float);
    vertexBuffer = Buffer(*device, vertexBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* data;
        vkMapMemory(device->logical, vertexBuffer.memory, 0, vertexBufferSize, 0, &data);
        memcpy(data, vertices.data(), vertexBufferSize);
        vkUnmapMemory(device->logical, vertexBuffer.memory);
    }

    const VkDeviceAddress vertexBufferDeviceAddress = vertexBuffer.getDeviceAddress(device->logical);

    const VkDeviceSize indexBufferSize = indices.size() * sizeof(unsigned int);
    indexBuffer = Buffer(*device, indexBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* data;
        vkMapMemory(device->logical, indexBuffer.memory, 0, indexBufferSize, 0, &data);
        memcpy(data, indices.data(), indexBufferSize);
        vkUnmapMemory(device->logical, indexBuffer.memory);
    }

    const VkDeviceAddress indexBufferDeviceAddress = indexBuffer.getDeviceAddress(device->logical);

    VkTransformMatrixKHR transform = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };

    const VkDeviceSize transformBufferSize = sizeof(transform);
    transformBuffer = Buffer(*device, transformBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* data;
        vkMapMemory(device->logical, transformBuffer.memory, 0, transformBufferSize, 0, &data);
        memcpy(data, indices.data(), transformBufferSize);
        vkUnmapMemory(device->logical, transformBuffer.memory);
    }

    const VkDeviceAddress transformBufferDeviceAddress = transformBuffer.getDeviceAddress(device->logical);

    VkAccelerationStructureGeometryTrianglesDataKHR accelerationStructureGeometryTrianglesData = {
        .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
        .pNext         = nullptr,
        .vertexFormat  = VK_FORMAT_R32G32B32_SFLOAT,
        .vertexData    = { vertexBufferDeviceAddress },
        .vertexStride  = sizeof(float) * 3,
        .maxVertex     = (uint32_t)vertices.size() / 3 - 1,
        .indexType     = VK_INDEX_TYPE_UINT32,
        .indexData     = { indexBufferDeviceAddress },
        .transformData = { transformBufferDeviceAddress }
    };

    VkAccelerationStructureGeometryKHR accelerationStructureGeometry = {
        .sType        = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
        .pNext        = nullptr,
        .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
        .geometry     = { accelerationStructureGeometryTrianglesData },
        .flags        = VK_GEOMETRY_OPAQUE_BIT_KHR
    };

    VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo = {
        .sType         = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
        .pNext         = nullptr,
        .type          = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        .flags         = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
        .geometryCount = 1,
        .pGeometries   = &accelerationStructureGeometry
    };

    uint32_t maxPrimitiveCount = indices.size() / 3;

    VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo = { VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    vkGetAccelerationStructureBuildSizes(device->logical, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &maxPrimitiveCount, &accelerationStructureBuildSizesInfo);

    const VkDeviceSize blasBufferSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    blasBuffer = Buffer(*device, blasBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo = {
        .sType       = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
        .pNext       = nullptr,
        .createFlags = 0,
        .buffer      = blasBuffer,
        .offset      = 0,
        .size        = blasBufferSize,
        .type        = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR
    };

    vkCreateAccelerationStructure(device->logical, &accelerationStructureCreateInfo, nullptr, &blas);

    Buffer scratchBuffer(*device, accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    accelerationStructureBuildGeometryInfo.mode                     = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = blas;
    accelerationStructureBuildGeometryInfo.scratchData              = { scratchBuffer.getDeviceAddress(device->logical) };

    VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo = {
        .primitiveCount  = maxPrimitiveCount,
        .primitiveOffset = 0,
        .firstVertex     = 0,
        .transformOffset = 0
    };

    VkAccelerationStructureBuildRangeInfoKHR* accelerationStructureBuildRangeInfos[] = { &accelerationStructureBuildRangeInfo };

    VkCommandBufferBeginInfo commandBufferBeginInfo = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext            = nullptr,
        .flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    vkCmdBuildAccelerationStructures(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos);

    vkEndCommandBuffer(commandBuffer);

    VkCommandBufferSubmitInfo commandBufferSubmitInfo = {
        .sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .pNext         = nullptr,
        .commandBuffer = commandBuffer,
        .deviceMask    = 0
    };

    VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .pNext                    = nullptr,
        .flags                    = 0,
        .waitSemaphoreInfoCount   = 0,
        .pWaitSemaphoreInfos      = nullptr,
        .commandBufferInfoCount   = 1,
        .pCommandBufferInfos      = &commandBufferSubmitInfo,
        .signalSemaphoreInfoCount = 0,
        .pSignalSemaphoreInfos    = nullptr
    };

    vkQueueSubmit2(device->renderQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->renderQueue);

    scratchBuffer.destroy(device->logical);

    VkAccelerationStructureDeviceAddressInfoKHR accelerationStructureDeviceAddressInfo = {
        .sType                 = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
        .pNext                 = nullptr,
        .accelerationStructure = blas
    };

    blasDeviceAddress = vkGetAccelerationStructureDeviceAddress(device->logical, &accelerationStructureDeviceAddressInfo);

    VkAccelerationStructureInstanceKHR accelerationStructureInstance = {
        .transform                              = transform,
        .instanceCustomIndex                    = 0,
        .mask                                   = 0xFF,
        .instanceShaderBindingTableRecordOffset = 0,
        .flags                                  = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
        .accelerationStructureReference         = blasDeviceAddress
    };

    const VkDeviceSize instanceBufferSize = sizeof(accelerationStructureInstance);
    instanceBuffer = Buffer(*device, instanceBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    {
        void* data;
        vkMapMemory(device->logical, instanceBuffer.memory, 0, instanceBufferSize, 0, &data);
        memcpy(data, &accelerationStructureInstance, instanceBufferSize);
        vkUnmapMemory(device->logical, instanceBuffer.memory);
    }

    const VkDeviceAddress instanceBufferDeviceAddress = instanceBuffer.getDeviceAddress(device->logical);

    VkAccelerationStructureGeometryInstancesDataKHR accelerationStructureGeometryInstancesData = {
        .sType           = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .pNext           = nullptr,
        .arrayOfPointers = VK_FALSE,
        .data            = { instanceBufferDeviceAddress }
    };

    accelerationStructureGeometry.geometryType       = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    accelerationStructureGeometry.geometry.instances = accelerationStructureGeometryInstancesData;

    accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    maxPrimitiveCount = 1;

    vkGetAccelerationStructureBuildSizes(device->logical, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &accelerationStructureBuildGeometryInfo, &maxPrimitiveCount, &accelerationStructureBuildSizesInfo);

    const VkDeviceSize tlasBufferSize = accelerationStructureBuildSizesInfo.accelerationStructureSize;
    tlasBuffer = Buffer(*device, tlasBufferSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    accelerationStructureCreateInfo.buffer = tlasBuffer;
    accelerationStructureCreateInfo.size   = tlasBufferSize;
    accelerationStructureCreateInfo.type   = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

    vkCreateAccelerationStructure(device->logical, &accelerationStructureCreateInfo, nullptr, &tlas);

    scratchBuffer = Buffer(*device, accelerationStructureBuildSizesInfo.buildScratchSize, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    accelerationStructureBuildGeometryInfo.dstAccelerationStructure = tlas;
    accelerationStructureBuildGeometryInfo.scratchData              = { scratchBuffer.getDeviceAddress(device->logical) };

    accelerationStructureBuildRangeInfo.primitiveCount = maxPrimitiveCount;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    vkCmdBuildAccelerationStructures(commandBuffer, 1, &accelerationStructureBuildGeometryInfo, accelerationStructureBuildRangeInfos);

    vkEndCommandBuffer(commandBuffer);

    vkQueueSubmit2(device->renderQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(device->renderQueue);

    scratchBuffer.destroy(device->logical);

    accelerationStructureDeviceAddressInfo.accelerationStructure = tlas;

    tlasDeviceAddress = vkGetAccelerationStructureDeviceAddress(device->logical, &accelerationStructureDeviceAddressInfo);
}
