#pragma once

#include "GLTFScene.h"
#include "SceneNode.h"

namespace RenderSys
{
    
class Scene
{
public:
    Scene();
    ~Scene() = default;
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void allocateMemory();
    void populate();
    void prepareNodeGraph();

    std::vector<Model::Vertex> m_vertexBuffer;
    std::vector<uint32_t> m_indexBuffer;
private:
    std::unique_ptr<GLTFScene> m_scene;
    std::shared_ptr<SceneNode> m_nodeGraph;
};

}

