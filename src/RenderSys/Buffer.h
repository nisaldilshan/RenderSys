#pragma once
#include <vector>
#include <memory>

#define GLM_FORCE_LEFT_HANDED
#include <glm/ext.hpp>
#include <glm/gtx/quaternion.hpp>

namespace RenderSys
{

#if (RENDERER_BACKEND == 1)
class OpenGLBuffer;
typedef OpenGLBuffer BufferType;
#elif (RENDERER_BACKEND == 2)
class VulkanBuffer;
typedef VulkanBuffer BufferType;
#elif (RENDERER_BACKEND == 3)
class WebGPUBuffer;
typedef WebGPUBuffer BufferType;
#else
static_assert(false);
#endif

struct alignas(16) Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoord0;
    glm::vec3 color;
    glm::vec3 tangent;
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


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

enum class BufferUsage
{
    UNIFORM_BUFFER_VISIBLE_TO_CPU,
    STORAGE_BUFFER_VISIBLE_TO_CPU,
    TRANSFER_SRC_VISIBLE_TO_GPU
};

class Buffer
{
public:
    Buffer(uint32_t byteSize, RenderSys::BufferUsage bufferUsage);
    ~Buffer();

    Buffer(const Buffer &) = delete;
    Buffer &operator=(const Buffer &) = delete;
    Buffer(Buffer &&) = delete;
    Buffer &operator=(Buffer &&) = delete;

    void MapBuffer();
    void WriteToBuffer(const void *data);
    bool Flush();

    std::shared_ptr<BufferType> GetPlatformBuffer() const { return m_platformBuffer; }
private:
    std::shared_ptr<BufferType> m_platformBuffer;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////





} // namespace RenderSys