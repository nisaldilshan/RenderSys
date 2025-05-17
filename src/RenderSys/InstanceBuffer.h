#pragma once
#include <RenderSys/Buffer.h>
namespace RenderSys
{

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

    void SetInstanceData(uint32_t index, glm::mat4 const& modelMatrix);
    void Update();
    const glm::mat4& GetModelMatrix(uint32_t index) const;
    std::shared_ptr<Buffer> GetBuffer() const { return m_buffer; }
    
private:
    std::shared_ptr<Buffer> m_buffer;
    bool m_Dirty;
    std::vector<InstanceData> m_DataInstances;
};

} // namespace RenderSys