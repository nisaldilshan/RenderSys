#pragma once
#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

namespace RenderSys
{

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord0;
    glm::vec2 texcoord1;
    glm::vec3 color;
    glm::uvec4 joint0;
    glm::vec4 weight0;
};

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
    Map,
    Uniform
};
    
} // namespace Compute
} // namespace RenderSys