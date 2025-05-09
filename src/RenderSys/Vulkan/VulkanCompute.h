#pragma once

#include <vector>
#include <vk_mem_alloc.h>
#include <Walnut/GraphicsAPI/VulkanGraphics.h>

#include <RenderSys/Buffer.h>
#include <RenderSys/RenderUtil.h>
#include <RenderSys/Shader.h>

namespace GraphicsAPI
{
    struct VulkanComputeBuffer
    {
        VkDescriptorBufferInfo buffer{VK_NULL_HANDLE, 0, 0};
        VmaAllocation bufferMemory = VK_NULL_HANDLE;
        VkDescriptorType type = VK_DESCRIPTOR_TYPE_MAX_ENUM;
    };
    struct MappedBuffer 
    {
        VulkanComputeBuffer gpuBuffer;
        VulkanComputeBuffer mapBuffer;
        std::vector<uint8_t> mappedData;
    };

    class VulkanCompute
    {
    public:
        VulkanCompute();
        ~VulkanCompute();

        void Init();
        void CreateBindGroup(const std::vector<RenderSys::BindGroupLayoutEntry>& bindGroupLayoutEntries);
        void CreateShaders(RenderSys::Shader& shader);
        void CreatePipeline();
        void CreateBuffer(uint32_t binding, uint32_t bufferLength, RenderSys::ComputeBuf::BufferType type);
        void SetBufferData(uint32_t binding, const void *bufferData, uint32_t bufferLength);
        void BeginComputePass();
        void Compute(const uint32_t workgroupCountX, const uint32_t workgroupCountY);
        void EndComputePass();
        std::vector<uint8_t>& GetMappedResult(uint32_t binding);
        void Destroy();
    private:
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStageInfos;
        std::unordered_map<std::string, std::vector<uint32_t>> m_shaderMap;

        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_pipeline = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;

        VkDescriptorSetLayout m_bindGroupLayout = VK_NULL_HANDLE;
        VkDescriptorPool m_bindGroupPool = VK_NULL_HANDLE;
        VkDescriptorSet m_bindGroup = VK_NULL_HANDLE;
        std::vector<VkDescriptorSetLayoutBinding> m_bindGroupBindings;

        std::unordered_map<uint32_t, VulkanComputeBuffer> m_buffersAccessibleToShader;
        std::unordered_map<uint32_t, std::shared_ptr<MappedBuffer>> m_shaderOutputBuffers;

        VmaAllocator m_vma = VK_NULL_HANDLE;
    };
}