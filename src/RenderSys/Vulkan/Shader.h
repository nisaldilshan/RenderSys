#pragma once
#include <vector>

#include <shaderc/shaderc.hpp>

// Code copied from - https://github.com/google/shaderc/blob/main/examples/online-compile/main.cc

namespace VulkanShaderUtils
{

    // Compiles a shader to a SPIR-V binary. Returns the binary as
    // a vector of 32-bit words.
    std::vector<uint32_t> compile_file(const std::string &source_name,
                                       shaderc_shader_kind kind,
                                       const std::string &source,
                                       bool optimize = false)
    {
        shaderc::Compiler compiler;
        shaderc::CompileOptions options;

        // Like -DMY_DEFINE=1
        options.AddMacroDefinition("MY_DEFINE", "1");
        if (optimize)
            options.SetOptimizationLevel(shaderc_optimization_level_size);

        shaderc::SpvCompilationResult module =
            compiler.CompileGlslToSpv(source, kind, source_name.c_str(), options);

        if (module.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            std::cerr << module.GetErrorMessage();
            return std::vector<uint32_t>();
        }

        return {module.cbegin(), module.cend()};
    }

} // namespace VulkanShaderUtils
