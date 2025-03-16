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

class GLTFScene
{
public:
    GLTFScene();
    ~GLTFScene();
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void computeProps();
    size_t getVertexCount() const;
    size_t getIndexCount() const;
    void loadTextures(std::vector<Texture>& textures);
    void loadTextureSamplers(std::vector<TextureSampler>& samplers);
    void loadMaterials(std::vector<Material>& materials);
    void loadJointData(std::vector<glm::tvec4<uint16_t>>& jointVec, std::vector<int>& nodeToJoint, std::vector<glm::vec4>& weightVec);
    void loadInverseBindMatrices(std::vector<glm::mat4>& inverseBindMatrices);
    void getNodeGraphs(std::vector<std::shared_ptr<Model::ModelNode>>& rootNodes);
private:
    static void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
    RenderSys::Primitive loadPrimitive(const tinygltf::Primitive &primitive, Model::ModelData &modelData, const uint32_t indexCount);
    RenderSys::Mesh loadMesh(const tinygltf::Mesh& gltfMesh, Model::ModelData &modelData, const uint32_t indexCount);
    std::shared_ptr<Model::ModelNode> traverse(const std::shared_ptr<Model::ModelNode> parent, const tinygltf::Node &node, uint32_t nodeIndex, uint32_t& indexCount);

    std::shared_ptr<tinygltf::Model> m_model;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
    std::filesystem::path m_sceneFilePath;
};

}

