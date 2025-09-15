#include "Shader.h"

#include <assert.h>
#include <fstream>
#include <string>
#include <shaderc/shaderc.hpp>

#if (RENDERER_BACKEND == 1) // OpenGL
static_assert(false);
#elif (RENDERER_BACKEND == 2) // Vulkan
#define USE_SHADERC
#elif (RENDERER_BACKEND == 3) // WebGPU
#else
static_assert(false);
#endif

namespace RenderSys
{

namespace ShaderUtils
{

#if defined(USE_SHADERC)

class MyIncluder : public shaderc::CompileOptions::IncluderInterface
{
public:
    MyIncluder(const std::string &includeDir) : includeDir_(includeDir) {}

    shaderc_include_result *GetInclude(const char *requested_source, shaderc_include_type type, const char *requesting_source, size_t include_depth) override
    {
        std::cout   << "Shader::GetInclude: requested_source=" << requested_source 
                    << ", type=" << (type == shaderc_include_type_relative ? "shaderc_include_type_relative" : "shaderc_include_type_standard")
                    << ", requesting_source=" << requesting_source 
                    << ", include_depth=" << include_depth 
                    << std::endl;
        auto fullPath = std::filesystem::path(includeDir_) / requested_source;
        std::ifstream file(fullPath, std::ios::binary);
        std::vector<char> content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
        if (content.empty())
        {
            return new shaderc_include_result{requested_source, strlen(requested_source), nullptr, 0, nullptr};
        }

        char *content_ptr = new char[content.size()];
        memcpy(content_ptr, content.data(), content.size());

        return new shaderc_include_result{requested_source, strlen(requested_source), content_ptr, content.size(), nullptr};
    }

    void ReleaseInclude(shaderc_include_result *data) override
    {
        delete[] data->content;
        delete data;
    }

private:
    std::string includeDir_;

    // std::string readFile(const std::string &filename)
    // {
    //     std::ifstream file(filename, std::ios::ate | std::ios::binary);
    //     if (!file.is_open())
    //     {
    //         return "";
    //     }
    //     size_t fileSize = (size_t)file.tellg();
    //     std::vector<char> buffer(fileSize);
    //     file.seekg(0);
    //     file.read(buffer.data(), fileSize);
    //     file.close();
    //     return std::string(buffer.begin(), buffer.end());

    //     return "";
    // }
};

// Compiles a shader to a SPIR-V binary. Returns the binary as
// a vector of 32-bit words.
std::vector<uint32_t> compile_file(const std::string &name,
                                    Shader& shader,
                                    bool optimize)
{
    shaderc_shader_kind kind;
    if (shader.stage == RenderSys::ShaderStage::Vertex)
    {
        kind = shaderc_glsl_vertex_shader;
    }
    else if (shader.stage == RenderSys::ShaderStage::Fragment)
    {
        kind = shaderc_glsl_fragment_shader;
    }
    else if (shader.stage == RenderSys::ShaderStage::Compute)
    {
        kind = shaderc_glsl_compute_shader;
    }
    else
    {
        assert(false);
    }

    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetGenerateDebugInfo();
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_0);
    if (!shader.GetIncludeDirectory().empty())
    {
        options.SetIncluder(std::make_unique<MyIncluder>(shader.GetIncludeDirectory()));
    }

    // Like -DMY_DEFINE=1
    options.AddMacroDefinition("MY_DEFINE", "1");
    if (optimize)
        options.SetOptimizationLevel(shaderc_optimization_level_size);

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(shader.GetShaderSrc(), kind, name.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << module.GetErrorMessage();
        return std::vector<uint32_t>();
    }

    return {module.cbegin(), module.cend()};
}

#else
std::vector<uint32_t> compile_file(const std::string &name, Shader& shader, bool optimize)
{
    return std::vector<uint32_t>();
}
#endif

} // namespace ShaderUtils

Shader::Shader(const std::string &name, const std::string &shaderSrc)
    : m_name(name), m_shaderSrc(shaderSrc)
{}

Shader::Shader(const std::filesystem::path &filePath)
{
    assert(!std::filesystem::exists(filePath));
    std::ifstream file(filePath, std::ios::binary);
    std::vector<char> content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());

    if (!file.is_open()) {
        std::cerr << "Unable to open file - " << filePath << std::endl;
        assert(false);
    }

    m_name = filePath.filename().string();
    m_shaderSrc = std::string(content.data(), content.size());
    SetIncludeDirectory(filePath.parent_path().string());
}

bool Shader::Compile(bool optimize)
{
    m_compiledShader = ShaderUtils::compile_file(m_name, *this, optimize);
    return m_compiledShader.size() > 0;
}

const std::vector<uint32_t>& Shader::GetCompiledShader() const
{
    return m_compiledShader;
}

void Shader::SetIncludeDirectory(const std::string &dir)
{
    m_includeDir = dir;
}

std::string Shader::GetIncludeDirectory() const
{
    return m_includeDir;
}

} // namespace RenderSys