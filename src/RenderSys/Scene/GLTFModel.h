#pragma once

#include <string>
#include <memory>
#include <filesystem>
#include <tiny_gltf.h>

#include <RenderSys/Texture.h>
#include <RenderSys/Material.h>
#include <RenderSys/Scene/Mesh.h>

namespace tinygltf
{
class Model;
class Node;
class Mesh;
class Primitive;
}

namespace RenderSys
{
class MeshData;
class Scene;
class GLTFModel
{
public:
    GLTFModel(Scene& scene);
    ~GLTFModel();
    GLTFModel(const GLTFModel&) = delete;
    GLTFModel &operator=(const GLTFModel&) = delete;
    GLTFModel(GLTFModel&&) = delete;
    GLTFModel &operator=(GLTFModel&&) = delete;
    bool load(const std::filesystem::path &filePath);
    void computeProps();
    
    void loadTextures();
    void loadMaterials();
    void loadJointData();
    void loadInverseBindMatrices();
    void loadjointMatrices();
    void loadSkeletons();
    void loadAnimations();
    void applyVertexSkinning(RenderSys::VertexBuffer& vertexBuffer);
    void getNodeGraphs();
    void printNodeGraph() const;
    const std::vector<std::shared_ptr<RenderSys::Texture>>& GetTextures() const { return m_textures; }
    const std::vector<std::shared_ptr<RenderSys::Material>>& GetMaterials() const { return m_materials; }
private:
    void loadTransform(entt::entity& nodeEntity, const tinygltf::Node &gltfNode, const uint32_t parent);
    std::vector<TextureSampler> loadTextureSamplers();
    RenderSys::SubMesh loadPrimitive(const tinygltf::Primitive &primitive, std::shared_ptr<MeshData> modelData, const uint32_t indexCount);
    void loadMesh(const tinygltf::Mesh& gltfMesh, entt::entity& nodeEntity);
    void traverse(const uint32_t parent, uint32_t nodeIndex);
    std::shared_ptr<RenderSys::Material> createMaterial(int materialIndex);

    template <typename T>
    int LoadAccessor(const tinygltf::Accessor& accessor, const T*& pointer, uint32_t* count = nullptr, int* type = nullptr)
    {
        const tinygltf::BufferView& view = m_gltfModel->bufferViews[accessor.bufferView];
        pointer =
            reinterpret_cast<const T*>(&(m_gltfModel->buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
        if (count)
        {
            *count = static_cast<uint32_t>(accessor.count);
        }
        if (type)
        {
            *type = accessor.type;
        }
        return accessor.componentType;
    }

    Scene& m_sceneRef;
    std::unique_ptr<tinygltf::Model> m_gltfModel;
    std::filesystem::path m_modelFilePath;

    std::vector<std::shared_ptr<RenderSys::Texture>> m_textures;
    std::vector<std::shared_ptr<RenderSys::Material>> m_materials;
    std::vector<int> m_nodeToJoint;
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<glm::mat4> m_jointMatrices;
    std::vector<glm::tvec4<uint16_t>> m_jointVec;
    std::vector<glm::vec4> m_weightVec;
};

}

