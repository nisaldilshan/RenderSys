#pragma once

#include <string>
#include <memory>
#include <filesystem>

namespace tinygltf
{
class Model;
class Node;
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
private:
    static void getNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount);
    std::shared_ptr<tinygltf::Model> m_model;
    size_t m_vertexCount = 0;
    size_t m_indexCount = 0;
};

}

