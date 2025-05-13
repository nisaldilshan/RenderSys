#pragma once
#include <RenderSys/Buffer.h>

namespace RenderSys
{

constexpr size_t MAX_INSTANCES = 32;
struct InstanceData
{
    glm::mat4 m_ModelMatrix;
    //glm::mat4 m_NormalMatrix;
};
    
class InstanceBuffer
{
public:
    InstanceBuffer();
    ~InstanceBuffer();

    InstanceBuffer(const InstanceBuffer &) = delete;
    InstanceBuffer &operator=(const InstanceBuffer &) = delete;
    InstanceBuffer(InstanceBuffer &&) = delete;
    InstanceBuffer &operator=(InstanceBuffer &&) = delete;

    void SetInstanceData(uint32_t index, glm::mat4 const& mat4Global);
    std::shared_ptr<Buffer> GetBuffer() const { return m_buffer; }
    
private:
    std::shared_ptr<Buffer> m_buffer;
    bool m_Dirty;
    std::vector<InstanceData> m_DataInstances;
};

} // namespace RenderSys