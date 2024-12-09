#pragma once

#include <string>
#include <memory>
#include <filesystem>

namespace tinygltf
{
class Model;
}

namespace RenderSys
{

class GLTFScene
{
public:
    GLTFScene();
    ~GLTFScene();
    bool load(const std::filesystem::path &filePath, const std::string& textureFilename);
private:
    std::shared_ptr<tinygltf::Model> m_model;
};

}

