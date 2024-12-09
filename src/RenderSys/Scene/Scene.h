#pragma once
#include <glm/ext.hpp>
#include "GLTFScene.h"

namespace RenderSys
{

namespace Model
{
    struct Vertex {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 uv0;
        glm::vec2 uv1;
        glm::uvec4 joint0;
        glm::vec4 weight0;
        glm::vec4 color;
    };
}
    
class Scene
{
public:
    Scene();
    ~Scene() = default;
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
    void allocateMemory();
private:
    std::unique_ptr<GLTFScene> m_scene;
    std::vector<Model::Vertex> m_vertexBuffer;
    std::vector<uint32_t> m_indexBuffer;
};

}

