#pragma once
#include <stdint.h>
#include <RenderSys/Resource.h>

namespace RenderSys
{

class WebGPUResourceDescriptor
{
public:
    WebGPUResourceDescriptor();
    ~WebGPUResourceDescriptor();

    void AttachBuffer(uint32_t binding, const std::shared_ptr<RenderSys::WebGPUBuffer>& buffer);
    void Init();

    //VkDescriptorSet m_bindGroup = VK_NULL_HANDLE; 
private:
    std::vector<void*> m_Writes;
};


} // namespace RenderSys