#include "scene.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

Scene::Scene(Device* device) : device(device) {
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
        .flags         = 0,
        .geometryCount = 1,
        .pGeometries   = &accelerationStructureGeometry
    };

    const uint32_t maxPrimitiveCount = indices.size() / 3;

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
}
