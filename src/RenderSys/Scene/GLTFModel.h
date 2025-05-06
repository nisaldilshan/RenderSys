#pragma once

#include <string>
#include <memory>
#include <filesystem>

#include "ModelNode.h"
#include <RenderSys/Texture.h>

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
class GLTFModel
{
public:
    GLTFModel(entt::registry& registry);
    ~GLTFModel();
    GLTFModel(const GLTFModel&) = delete;
    GLTFModel &operator=(const GLTFModel&) = delete;
    GLTFModel(GLTFModel&&) = delete;
    GLTFModel &operator=(GLTFModel&&) = delete;
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void computeProps();
    
    void loadTextures();
    void loadMaterials();
    void loadJointData(std::vector<glm::tvec4<uint16_t>>& jointVec, std::vector<glm::vec4>& weightVec);
    void loadInverseBindMatrices();
    void loadjointMatrices(std::vector<std::shared_ptr<ModelNode>>& rootNodes);

    void getNodeGraphs(std::vector<std::shared_ptr<ModelNode>>& rootNodes);
    const std::vector<std::shared_ptr<RenderSys::Texture>>& GetTextures() const { return m_textures; }
    const std::vector<std::shared_ptr<RenderSys::Material>>& GetMaterials() const { return m_materials; }
    const std::vector<glm::mat4>& GetJointMatrices() const { return m_jointMatrices; }
private:
    void loadTransform(std::shared_ptr<ModelNode> currentNode, const tinygltf::Node &gltfNode, std::shared_ptr<ModelNode> parentNode);
    std::vector<TextureSampler> loadTextureSamplers();
    RenderSys::SubMesh loadPrimitive(const tinygltf::Primitive &primitive, std::shared_ptr<MeshData> modelData, const uint32_t indexCount);
    void loadMesh(const tinygltf::Mesh& gltfMesh, std::shared_ptr<ModelNode> node, const uint32_t indexCount);
    std::shared_ptr<ModelNode> traverse(const std::shared_ptr<ModelNode> parent, const tinygltf::Node &node, uint32_t nodeIndex, uint32_t& indexCount);
    std::shared_ptr<RenderSys::Material> createMaterial(int materialIndex);

    entt::registry& m_registryRef;
    std::unique_ptr<tinygltf::Model> m_gltfModel;
    std::filesystem::path m_sceneFilePath;

    std::vector<std::shared_ptr<RenderSys::Texture>> m_textures;
    std::vector<std::shared_ptr<RenderSys::Material>> m_materials;
    std::vector<int> m_nodeToJoint;
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<glm::mat4> m_jointMatrices;
};

}

