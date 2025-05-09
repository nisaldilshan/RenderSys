#pragma once
#include <filesystem>
#include <entt/entt.hpp>
#include <RenderSys/Buffer.h>

namespace RenderSys
{
    
class GLTFModel;
class Model
{
public:
    Model() = delete;
    Model(entt::registry& registry);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;
    
    bool load(const std::filesystem::path &filePath);
    void populate();
    void printNodeGraph();
    void applyVertexSkinning(RenderSys::VertexBuffer& vertexBuffer);
    const std::vector<std::shared_ptr<Texture>>& getTextures() const;
    const std::vector<std::shared_ptr<RenderSys::Material>>& getMaterials() const;

private:
    GLTFModel* m_model;
};

}

