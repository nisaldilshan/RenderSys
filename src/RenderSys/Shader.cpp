#include "Shader.h"

#include <assert.h>
#include <shaderc/shaderc.hpp>

namespace RenderSys
{

namespace ShaderUtils
{

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
    else
    {
        assert(false);
    }
    const std::string &source = shader.shaderSrc;
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetGenerateDebugInfo();
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
    options.SetTargetSpirv(shaderc_spirv_version_1_0);

    // Like -DMY_DEFINE=1
    options.AddMacroDefinition("MY_DEFINE", "1");
    if (optimize)
        options.SetOptimizationLevel(shaderc_optimization_level_size);

    shaderc::SpvCompilationResult module =
        compiler.CompileGlslToSpv(source, kind, name.c_str(), options);

    if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    {
        std::cerr << module.GetErrorMessage();
        return std::vector<uint32_t>();
    }

    return {module.cbegin(), module.cend()};
}

} // namespace ShaderUtils

}