#pragma once

#include <string>
#include <memory>

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
    bool load(const std::string& modelFilename, const std::string& textureFilename);
private:
    std::shared_ptr<tinygltf::Model> m_model;
};

}

