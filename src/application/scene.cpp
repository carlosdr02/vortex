#include "scene.h"

#include <iostream>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>

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

    std::cout << "Number of vertices: " << vertices.size() / 3 << std::endl;
    for (size_t i = 0; i < vertices.size(); i += 3) {
        std::cout << "Vertex " << i / 3 << ": [" << vertices[i] << ", " << vertices[i + 1] << ", " << vertices[i + 2] << "]" << std::endl;
    }

    std::cout << "Number of indices: " << indices.size() << std::endl;
    for (size_t i = 0; i < indices.size(); i += 3) {
        std::cout << "Triangle " << i / 3 << ": [" << indices[i] << ", " << indices[i + 1] << ", " << indices[i + 2] << "]" << std::endl;
    }
}
