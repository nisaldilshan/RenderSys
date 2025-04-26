#pragma once

#include "GLTFScene.h"
#include "SceneNode.h"
#include <RenderSys/Buffer.h>

namespace RenderSys
{
    
class Model
{
public:
    Model();
    ~Model() = default;
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = delete;
    Model& operator=(Model&&) = delete;
    
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void populate();
    void printNodeGraph();
    void applyVertexSkinning(RenderSys::VertexBuffer& vertexBuffer);
    const std::vector<std::shared_ptr<Texture>>& getTextures() const { return m_scene->GetTextures(); }
    const std::vector<std::shared_ptr<RenderSys::Material>>& getMaterials() const { return m_scene->GetMaterials(); }
    const std::vector<std::shared_ptr<ModelNode>>& getRootNodes() const { return m_rootNodes; }
    
    std::vector<glm::tvec4<uint16_t>> m_jointVec;
    std::vector<int> m_nodeToJoint;
    std::vector<glm::vec4> m_weightVec;
    std::vector<glm::mat4> m_inverseBindMatrices;
    std::vector<glm::mat4> m_jointMatrices;

private:
    std::vector<std::shared_ptr<ModelNode>> m_rootNodes;
    std::unique_ptr<GLTFModel> m_scene;
};

}

