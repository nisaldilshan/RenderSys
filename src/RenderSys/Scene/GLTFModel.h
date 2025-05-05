#pragma once

#include <string>
#include <memory>
#include <filesystem>

#include "SceneNode.h"
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

class GLTFModel
{
public:
    GLTFModel();
    ~GLTFModel();
    GLTFModel(const GLTFModel&) = delete;
    GLTFModel &operator=(const GLTFModel&) = delete;
    GLTFModel(GLTFModel&&) = delete;
    GLTFModel &operator=(GLTFModel&&) = delete;
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void computeProps();
    size_t getVertexCount() const;
    size_t getIndexCount() const;
    
    void loadTextures();
    void loadMaterials();
    void loadJointData(std::vector<glm::tvec4<uint16_t>>& jointVec, std::vector<int>& nodeToJoint, std::vector<glm::vec4>& weightVec);
    void loadInverseBindMatrices(std::vector<glm::mat4>& inverseBindMatrices);
    void getNodeGraphs(std::vector<std::shared_ptr<ModelNode>>& rootNodes);
    const std::vector<std::shared_ptr<RenderSys::Texture>>& GetTextures() const { return m_textures; }
    const std::vector<std::shared_ptr<RenderSys::Material>>& GetMaterials() const { return m_materials; }
private:
    void loadTransform(std::shared_ptr<ModelNode> currentNode, const tinygltf::Node &gltfNode, std::shared_ptr<ModelNode> parentNode);
    std::vector<TextureSampler> loadTextureSamplers();
    static void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
    RenderSys::SubMesh loadPrimitive(const tinygltf::Primitive &primitive, std::shared_ptr<ModelData> modelData, const uint32_t indexCount);
    void loadMesh(const tinygltf::Mesh& gltfMesh, std::shared_ptr<ModelNode> node, const uint32_t indexCount);
    std::shared_ptr<ModelNode> traverse(const std::shared_ptr<ModelNode> parent, const tinygltf::Node &node, uint32_t nodeIndex, uint32_t& indexCount);
    std::shared_ptr<RenderSys::Material> createMaterial(int materialIndex);

    std::unique_ptr<tinygltf::Model> m_gltfModel;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
    std::filesystem::path m_sceneFilePath;
    std::vector<std::shared_ptr<RenderSys::Texture>> m_textures;
    std::vector<std::shared_ptr<RenderSys::Material>> m_materials;
};

}

