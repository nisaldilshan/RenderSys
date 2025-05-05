#pragma once
#include <vector>
#include <memory>
#include <string>
#include <glm/ext.hpp>
#include <entt/entt.hpp>
#include <RenderSys/Buffer.h>

namespace RenderSys
{

struct ModelVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::uvec4 joint0;
    glm::vec4 weight0;
    glm::vec4 color;
    glm::vec3 tangent;
};

struct ModelData 
{
    ModelData() = default;
    ~ModelData() = default;
    ModelData(const ModelData&) = delete;
    ModelData &operator=(const ModelData&) = delete;
    ModelData(ModelData&&) = delete;
    ModelData &operator=(ModelData&&) = delete;

    const RenderSys::VertexBuffer getVertexBufferForRenderer() const
    {
        RenderSys::VertexBuffer buffer;
        buffer.resize(vertices.size());
        for (size_t i = 0; i < buffer.size(); i++)
        {
            buffer[i].position = vertices[i].pos;
            buffer[i].normal = vertices[i].normal;
            buffer[i].texcoord0 = vertices[i].uv0;
            buffer[i].tangent = vertices[i].tangent;
        }
        return buffer;
    }

    std::vector<ModelVertex> vertices;
    std::vector<uint32_t> indices;
};

class Resource;
struct SubMesh
{
    uint32_t m_FirstIndex = 0;
    uint32_t m_FirstVertex = 0;
    uint32_t m_IndexCount = 0;
    uint32_t m_VertexCount = 0;
    uint32_t m_InstanceCount = 1;
    std::shared_ptr<Material> m_Material = nullptr;
    std::shared_ptr<Resource> m_Resource = nullptr;
};

struct Mesh
{
    Mesh(std::shared_ptr<ModelData> modelData)
        : vertexBufferID(0)
        , m_modelData(modelData)
    {}

    ~Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh &operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh &operator=(Mesh&&) = delete;

    uint32_t vertexBufferID = 0;
    std::shared_ptr<ModelData> m_modelData;
    std::vector<SubMesh> subMeshes;
};



class ModelNode 
{
  public:
    ModelNode() = delete; 
    ModelNode(int nodeNum, entt::entity entity);
    ~ModelNode() = default;

	ModelNode(const ModelNode&) = delete;
	ModelNode &operator=(const ModelNode&) = delete;
	ModelNode(ModelNode&&) = delete;
	ModelNode &operator=(ModelNode&&) = delete;

    int getNodeNum();
    void setNodeName(std::string name);
    entt::entity& getEntity();

    void calculateJointMatrices(const std::vector<glm::mat4>& inverseBindMatrices, const std::vector<int>& nodeToJoint
        , std::vector<glm::mat4>& jointMatrices, entt::registry& registry);
    void printHierarchy(int indent);

    std::vector<std::shared_ptr<ModelNode>> m_childNodes{};
    std::shared_ptr<ModelData> m_data;

  private:
    int m_nodeNum = 0;
    std::string m_nodeName;
    int m_materialIndex = 0;
    entt::entity m_entity;
};

} // namespace RenderSys