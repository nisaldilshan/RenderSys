#include "Shader.h"

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
    shaderc_shader_kind kind = shaderc_glsl_vertex_shader;
    const std::string &source = shader.shaderSrc;
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;

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