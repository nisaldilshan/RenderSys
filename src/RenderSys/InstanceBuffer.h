#pragma once
#include <RenderSys/Buffer.h>

namespace RenderSys
{
    
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
};

} // namespace RenderSys