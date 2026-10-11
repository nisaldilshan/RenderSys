// Renders a triangle on a Vulkan device RenderSys creates itself (no window, no Walnut),
// reads the image back and checks a few pixels. Exits with 0 on success.
// For validation, set VK_LAYER_PATH to a vulkan-validationlayers 1.3.243+ bin folder: the 1.3.239.0
// package Walnut pulls in has a manifest the Vulkan loader skips.
#include <RenderSys/Renderer3D.h>
#include <RenderSys/Vulkan/VulkanContext.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>

namespace
{

struct Uniforms
{
    std::array<float, 4> color;
};
static_assert(sizeof(Uniforms) % 16 == 0);

constexpr uint32_t kWidth = 256;
constexpr uint32_t kHeight = 256;
constexpr std::array<uint8_t, 4> kClearColor = {25, 51, 76, 255};    // 0.1, 0.2, 0.3
constexpr std::array<uint8_t, 4> kTriangleColor = {255, 128, 0, 255}; // 1.0, ~0.5, 0.0

const char* kVertexShader = R"(
    #version 450 core
    layout(binding = 0) uniform Uniforms { vec4 color; } ubo;
    layout(location = 0) in vec3 aPos;
    void main() { gl_Position = vec4(aPos, 1.0); }
)";

const char* kFragmentShader = R"(
    #version 450
    layout(binding = 0) uniform Uniforms { vec4 color; } ubo;
    layout(location = 0) out vec4 FragColor;
    void main() { FragColor = ubo.color; }
)";

bool PixelMatches(const std::vector<uint8_t>& image, uint32_t x, uint32_t y, const std::array<uint8_t, 4>& expected)
{
    const uint8_t* pixel = &image[(y * kWidth + x) * 4];
    bool matches = true;
    for (int c = 0; c < 4; c++)
        matches &= std::abs(int(pixel[c]) - int(expected[c])) <= 2;
    printf("  pixel (%3u,%3u) = %3u %3u %3u %3u, expected %3u %3u %3u %3u: %s\n", x, y,
           pixel[0], pixel[1], pixel[2], pixel[3],
           expected[0], expected[1], expected[2], expected[3], matches ? "ok" : "MISMATCH");
    return matches;
}

bool RenderAndCheck()
{
    auto renderer = std::make_unique<RenderSys::Renderer3D>();
    renderer->Init();

    RenderSys::Shader vertexShader("Vertex", kVertexShader);
    vertexShader.type = RenderSys::ShaderType::SPIRV;
    vertexShader.stage = RenderSys::ShaderStage::Vertex;
    renderer->SetShader(vertexShader);

    RenderSys::Shader fragmentShader("Fragment", kFragmentShader);
    fragmentShader.type = RenderSys::ShaderType::SPIRV;
    fragmentShader.stage = RenderSys::ShaderStage::Fragment;
    renderer->SetShader(fragmentShader);

    // One triangle around the centre in clip space
    RenderSys::VertexBuffer vertices;
    vertices.resize(3);
    vertices[0].position = {-0.5f, -0.5f, 0.5f};
    vertices[1].position = { 0.5f, -0.5f, 0.5f};
    vertices[2].position = { 0.0f,  0.5f, 0.5f};

    RenderSys::VertexAttribute positionAttribute;
    positionAttribute.location = 0;
    positionAttribute.format = RenderSys::VertexFormat::Float32x3;
    positionAttribute.offset = offsetof(RenderSys::Vertex, position);

    RenderSys::VertexBufferLayout vertexLayout;
    vertexLayout.attributeCount = 1;
    vertexLayout.attributes = &positionAttribute;
    vertexLayout.arrayStride = sizeof(RenderSys::Vertex);
    vertexLayout.stepMode = RenderSys::VertexStepMode::Vertex;
    const auto vertexBufferID = renderer->SetVertexBufferData(vertices, vertexLayout);
    // Both windings, so the check does not depend on the renderer's cull mode
    renderer->SetIndexBufferData(vertexBufferID, {0, 1, 2, 0, 2, 1});

    std::vector<RenderSys::BindGroupLayoutEntry> bindings(1);
    bindings[0].setDefault();
    bindings[0].binding = 0;
    bindings[0].visibility = static_cast<RenderSys::ShaderStage>(
        static_cast<uint32_t>(RenderSys::ShaderStage::Vertex) | static_cast<uint32_t>(RenderSys::ShaderStage::Fragment));
    bindings[0].buffer.type = RenderSys::BufferBindingType::Uniform;
    bindings[0].buffer.minBindingSize = sizeof(Uniforms);
    bindings[0].buffer.hasDynamicOffset = false;
    renderer->CreateUniformBuffer(0, sizeof(Uniforms), 1);
    renderer->CreateBindGroup(bindings);
    renderer->CreatePipeline();

    renderer->SetClearColor({kClearColor[0] / 255.0f, kClearColor[1] / 255.0f, kClearColor[2] / 255.0f, 1.0f});
    renderer->OnResize(kWidth, kHeight);

    const Uniforms uniforms{{kTriangleColor[0] / 255.0f, kTriangleColor[1] / 255.0f, kTriangleColor[2] / 255.0f, 1.0f}};
    renderer->BeginFrame();
    renderer->BeginRenderPass();
    renderer->SetUniformBufferData(0, &uniforms, 0);
    renderer->BindResources();
    renderer->RenderIndexed(0);
    renderer->EndRenderPass();
    renderer->EndFrame();

    const std::vector<uint8_t>& image = renderer->GetRenderedImageData();
    bool ok = image.size() == kWidth * kHeight * 4;
    if (ok)
    {
        ok &= PixelMatches(image, kWidth / 2, kHeight / 2, kTriangleColor);
        ok &= PixelMatches(image, 2, 2, kClearColor);
        ok &= PixelMatches(image, kWidth - 3, kHeight - 3, kClearColor);
    }
    else
    {
        printf("  read back %zu bytes, expected %u\n", image.size(), kWidth * kHeight * 4);
    }

    renderer->Destroy();
    return ok;
}

} // namespace

int main()
{
    RenderSys::Vulkan::ContextCreateInfo createInfo;
    createInfo.applicationName = "HeadlessRender";
    createInfo.validation = true;
    if (!RenderSys::Vulkan::CreateContext(createInfo))
        return EXIT_FAILURE;

    const bool imageOk = RenderAndCheck();
    const uint32_t renderErrors = RenderSys::Vulkan::GetValidationErrorCount();

    RenderSys::Vulkan::DestroyContext();
    const uint32_t teardownErrors = RenderSys::Vulkan::GetValidationErrorCount() - renderErrors;

    printf("image %s, validation errors: %u while rendering, %u at teardown\n",
           imageOk ? "matches" : "DIFFERS", renderErrors, teardownErrors);
    return imageOk && renderErrors == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
