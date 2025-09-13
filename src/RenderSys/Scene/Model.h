#pragma once
#include <filesystem>
#include <RenderSys/Buffer.h>

namespace RenderSys
{
class Scene;
class GLTFModel;
class Texture;
class Material;

class Model
{
public:
    Model() = delete;
    Model(Scene& scene);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;
    
    bool load(const std::filesystem::path &filePath);
    void populate();
    void applyVertexSkinningOnCPU(RenderSys::VertexBuffer& vertexBuffer);
    const std::vector<std::shared_ptr<Texture>>& getTextures() const;
    const std::vector<std::shared_ptr<RenderSys::Material>>& getMaterials() const;

private:
    GLTFModel* m_model;
};

}

