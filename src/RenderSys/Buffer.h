#pragma once
#include <vector>
#include <memory>

#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

namespace RenderSys
{


enum class ShaderWorkflow : int32_t
{ 
    PBR_WORKFLOW_METALLIC_ROUGHNESS = 0, 
    PBR_WORKFLOW_SPECULAR_GLOSSINESS = 1 
};
static_assert(sizeof(ShaderWorkflow) == 4);

struct Material
{
    ShaderWorkflow workflow = ShaderWorkflow::PBR_WORKFLOW_METALLIC_ROUGHNESS;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    bool doubleSided = false;
    int baseColorTextureIndex = -1;
    int metallicRoughnessTextureIndex = -1;
    int normalTextureIndex = -1;
    struct TexCoordSets {
        uint8_t baseColor = 0;
        uint8_t normal = 0;
    } texCoordSets;
};

struct Primitive
{
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	bool hasIndices = false;
    uint32_t materialIndex = 0;
};

struct Mesh
{
	std::vector<Primitive> primitives;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord0;
    glm::vec3 color;
    glm::vec3 tangent;
    float _pad[2];
};

static_assert(sizeof(Vertex) % 16 == 0);

struct VertexBuffer
{
    std::vector<Vertex> vertices;
    void resize(size_t size) { vertices.resize(size); }
    size_t size() const { return vertices.size(); }
    void clear() { vertices.clear(); }
    Vertex& operator[](size_t index) { return vertices[index]; }
    const Vertex& operator[](size_t index) const { return vertices[index]; }
};

namespace ComputeBuf
{

enum class BufferType
{
    Input = 0,
    Output,
    Uniform
};
    
} // namespace Compute
} // namespace RenderSys