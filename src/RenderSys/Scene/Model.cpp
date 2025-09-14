#include "Model.h"
#include <iostream>
#include "GLTFModel.h"
#include <RenderSys/Material.h>

namespace RenderSys
{

Model::Model(Scene& scene)
    : m_model(new GLTFModel(scene))
{}

Model::~Model()
{
    delete m_model;
    m_model = nullptr;
}

bool Model::load(const std::filesystem::path &filePath)
{
    if (!m_model->load(filePath))
        return false;
    populate();
    return true;
}

void Model::populate()
{
    m_model->computeProps(); // just to print the number of vertices and indices
    m_model->loadTextures();
    m_model->loadMaterials();
    m_model->loadJointData();
    m_model->loadInverseBindMatrices();

    // traverse through GLTF Scene nodes and prepare graph
    m_model->getNodeGraphs();
    m_model->loadSkeletons();
    m_model->loadAnimations();
    m_model->loadjointMatrices();
}

void Model::applyVertexSkinningOnCPU(RenderSys::VertexBuffer& vertexBuffer)
{
    m_model->applyVertexSkinning(vertexBuffer);
}

const std::vector<std::shared_ptr<Texture>> &Model::getTextures() const
{
    return m_model->GetTextures();
}

const std::vector<std::shared_ptr<RenderSys::Material>> &Model::getMaterials() const
{
    return m_model->GetMaterials();
}

} // namespace RenderSys
