#pragma once
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

struct MeshData 
{
    MeshData() = default;
    ~MeshData() = default;
    MeshData(const MeshData&) = delete;
    MeshData &operator=(const MeshData&) = delete;
    MeshData(MeshData&&) = delete;
    MeshData &operator=(MeshData&&) = delete;

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
    Mesh(std::shared_ptr<MeshData> modelData)
        : vertexBufferID(0)
        , m_modelData(modelData)
    {}

    ~Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh &operator=(const Mesh&) = delete;
    Mesh(Mesh&&) = delete;
    Mesh &operator=(Mesh&&) = delete;

    uint32_t vertexBufferID = 0;
    std::shared_ptr<MeshData> m_modelData;
    std::vector<SubMesh> subMeshes;
};

} // namespace RenderSys
