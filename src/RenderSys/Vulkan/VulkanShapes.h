#pragma once

namespace RenderSys
{

namespace Vulkan
{

enum class ShapeType
{
    Plane = 0,
    Cube,
    Pyramid,
    Sphere
    // ... other shapes
};

std::unordered_map<ShapeType, std::shared_ptr<VulkanVertexIndexBufferInfo>> g_shapeInfoMap;

void InitShapes()
{
    // Create plane
    {
        std::vector<RenderSys::Vertex> vertices = {
            {glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}};
        std::vector<uint32_t> indices = {
            0, 1, 2,
            2, 3, 0};

        constexpr uint32_t binding = 0;
        auto planeInfo = std::make_shared<VulkanVertexIndexBufferInfo>();
        planeInfo->m_vertexCount = vertices.size();
        planeInfo->m_indexCount = indices.size();
        planeInfo->m_vertextBindingDescs = {
            binding, sizeof(RenderSys::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        planeInfo->m_vertextAttribDescs = {
            {0, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, position)},
            {1, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, normal)},
            {2, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x2), offsetof(RenderSys::Vertex, texcoord0)},
            {3, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, color)},
            {4, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, tangent)}};
        g_shapeInfoMap.insert({ShapeType::Plane, planeInfo});
    }
    // Create cube
    {
        std::vector<RenderSys::Vertex> vertices = {
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)},
            {glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)}};

        std::vector<uint32_t> indices = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            0, 4, 7, 7, 3, 0,
            1, 5, 6, 6, 2, 1,
            3, 2, 6, 6, 7, 3,
            0, 1, 5, 5, 4, 0};

        constexpr uint32_t binding = 0;
        auto cubeInfo = std::make_shared<VulkanVertexIndexBufferInfo>();
        cubeInfo->m_vertexCount = vertices.size();
        cubeInfo->m_indexCount = indices.size();
        cubeInfo->m_vertextBindingDescs = {
            binding, sizeof(RenderSys::Vertex), VK_VERTEX_INPUT_RATE_VERTEX};
        cubeInfo->m_vertextAttribDescs = {
            {0, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, position)},
            {1, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, normal)},
            {2, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x2), offsetof(RenderSys::Vertex, texcoord0)},
            {3, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, color)},
            {4, binding, RenderSys::Vulkan::RenderSysFormatToVulkanFormat(RenderSys::VertexFormat::Float32x3), offsetof(RenderSys::Vertex, tangent)}};
        g_shapeInfoMap.insert({ShapeType::Cube, cubeInfo});
    }
}

} // namespace Vulkan

} // namespace RenderSys
