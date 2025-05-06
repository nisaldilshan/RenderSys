#pragma once

#include "GLTFModel.h"
#include "ModelNode.h"
#include <RenderSys/Buffer.h>

namespace RenderSys
{
    
class Model
{
public:
    Model(entt::registry& registry);
    ~Model() = default;
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;
    
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void populate();
    void printNodeGraph();
    void applyVertexSkinning(RenderSys::VertexBuffer& vertexBuffer);
    const std::vector<std::shared_ptr<Texture>>& getTextures() const { return m_model->GetTextures(); }
    const std::vector<std::shared_ptr<RenderSys::Material>>& getMaterials() const { return m_model->GetMaterials(); }
    const std::vector<std::shared_ptr<ModelNode>>& getRootNodes() const { return m_rootNodes; }
    
    std::vector<glm::tvec4<uint16_t>> m_jointVec;
    std::vector<glm::vec4> m_weightVec;

private:
    std::vector<std::shared_ptr<ModelNode>> m_rootNodes;
    std::unique_ptr<GLTFModel> m_model;
};

}

