cmake_minimum_required (VERSION 3.23.0)
project (RenderSys)

find_package(walnut REQUIRED)
message(STATUS "RENDERER: " ${RENDERER})

if(RENDERER STREQUAL "Vulkan")
    include(FetchContent)
    set(VulkanMemoryAllocatorURL https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git)
    message(STATUS "Fetching: " ${VulkanMemoryAllocatorURL})
    FetchContent_Declare(
        vulkanmemoryallocator
        GIT_REPOSITORY ${VulkanMemoryAllocatorURL}
        GIT_TAG        v3.1.0
    )
    FetchContent_MakeAvailable(vulkanmemoryallocator) 
    message(STATUS "vulkanmemoryallocator sourceDir: ${vulkanmemoryallocator_SOURCE_DIR}")
    message(STATUS "vulkanmemoryallocator buildDir: ${vulkanmemoryallocator_BINARY_DIR}") 

    add_compile_definitions(RENDERER_BACKEND=2)
    find_package(shaderc REQUIRED)
    add_library (RenderSys2D STATIC
                    src/RenderSys/Renderer2D.cpp
                    src/RenderSys/Vulkan/VulkanRenderer2D.cpp
                    src/RenderSys/Shader.cpp
    )
    target_link_libraries(RenderSys2D PRIVATE GPUOpen::VulkanMemoryAllocator)
    target_link_libraries(RenderSys2D PRIVATE shaderc::shaderc)
    add_library (RenderSys3D STATIC
                    src/RenderSys/Renderer3D.cpp
                    src/RenderSys/Vulkan/VulkanRenderer3D.cpp
    )
    add_library (ComputeSys STATIC
                    src/RenderSys/Compute.cpp
                    src/RenderSys/Vulkan/VulkanCompute.cpp
    )
else()
    find_package(WebGPU REQUIRED)
    add_library(WEBGPU-WHOLEARCHIVE INTERFACE)
    target_link_directories(WEBGPU-WHOLEARCHIVE INTERFACE $<$<CONFIG:DEBUG>:${WebGPU_LIB_DIRS_DEBUG}>) # TODO: add lib dir for non debug configurations as well
    if(WIN32)
    target_link_libraries(WEBGPU-WHOLEARCHIVE INTERFACE
                            absl_bad_any_cast_impl.lib
                            absl_bad_optional_access.lib
                            absl_bad_variant_access.lib
                            absl_base.lib
                            absl_city.lib
                            absl_civil_time.lib
                            absl_cord.lib
                            absl_cordz_functions.lib
                            absl_cordz_handle.lib
                            absl_cordz_info.lib
                            absl_cordz_sample_token.lib
                            absl_cord_internal.lib
                            absl_crc32c.lib
                            absl_crc_cord_state.lib
                            absl_crc_cpu_detect.lib
                            absl_crc_internal.lib
                            absl_debugging_internal.lib
                            absl_demangle_internal.lib
                            absl_die_if_null.lib
                            absl_examine_stack.lib
                            absl_exponential_biased.lib
                            absl_failure_signal_handler.lib
                            absl_flags_commandlineflag.lib
                            absl_flags_commandlineflag_internal.lib
                            absl_flags_config.lib
                            absl_flags_internal.lib
                            absl_flags_marshalling.lib
                            absl_flags_parse.lib
                            absl_flags_private_handle_accessor.lib
                            absl_flags_program_name.lib
                            absl_flags_reflection.lib
                            absl_flags_usage.lib
                            absl_flags_usage_internal.lib
                            absl_graphcycles_internal.lib
                            absl_hash.lib
                            absl_hashtablez_sampler.lib
                            absl_int128.lib
                            absl_kernel_timeout_internal.lib
                            absl_leak_check.lib
                            absl_log_entry.lib
                            absl_log_flags.lib
                            absl_log_globals.lib
                            absl_log_initialize.lib
                            absl_log_internal_check_op.lib
                            absl_log_internal_conditions.lib
                            absl_log_internal_fnmatch.lib
                            absl_log_internal_format.lib
                            absl_log_internal_globals.lib
                            absl_log_internal_log_sink_set.lib
                            absl_log_internal_message.lib
                            absl_log_internal_nullguard.lib
                            absl_log_internal_proto.lib
                            absl_log_severity.lib
                            absl_log_sink.lib
                            absl_low_level_hash.lib
                            absl_malloc_internal.lib
                            absl_periodic_sampler.lib
                            absl_random_distributions.lib
                            absl_random_internal_distribution_test_util.lib
                            absl_random_internal_platform.lib
                            absl_random_internal_pool_urbg.lib
                            absl_random_internal_randen.lib
                            absl_random_internal_randen_hwaes.lib
                            absl_random_internal_randen_hwaes_impl.lib
                            absl_random_internal_randen_slow.lib
                            absl_random_internal_seed_material.lib
                            absl_random_seed_gen_exception.lib
                            absl_random_seed_sequences.lib
                            absl_raw_hash_set.lib
                            absl_raw_logging_internal.lib
                            absl_scoped_set_env.lib
                            absl_spinlock_wait.lib
                            absl_stacktrace.lib
                            absl_status.lib
                            absl_statusor.lib
                            absl_strerror.lib
                            absl_strings.lib
                            absl_strings_internal.lib
                            absl_string_view.lib
                            absl_str_format_internal.lib
                            absl_symbolize.lib
                            absl_synchronization.lib
                            absl_throw_delegate.lib
                            absl_time.lib
                            absl_time_zone.lib
                            absl_vlog_config_internal.lib
                            webgpu_dawn.lib
                            dawn_common.lib
                            dawn_native.lib
                            dawn_platform.lib
                            dawn_proc.lib
                            dawn_utils.lib
                            dawn_wire.lib
                            SPIRV-Tools-opt.lib
                            SPIRV-Tools.lib
                            tint_api.lib
                            tint_api_common.lib
                            tint_lang_core.lib
                            tint_lang_core_common.lib
                            tint_lang_core_constant.lib
                            tint_lang_core_intrinsic.lib
                            tint_lang_core_ir.lib
                            tint_lang_core_ir_transform.lib
                            tint_lang_core_ir_transform_common.lib
                            tint_lang_core_type.lib
                            tint_lang_hlsl_writer_common.lib
                            tint_lang_spirv.lib
                            tint_lang_spirv_intrinsic.lib
                            tint_lang_spirv_ir.lib
                            tint_lang_spirv_type.lib
                            tint_lang_spirv_writer.lib
                            tint_lang_spirv_writer_ast_printer.lib
                            tint_lang_spirv_writer_ast_raise.lib
                            tint_lang_spirv_writer_common.lib
                            tint_lang_spirv_writer_printer.lib
                            tint_lang_spirv_writer_raise.lib
                            tint_lang_wgsl.lib
                            tint_lang_wgsl_ast.lib
                            tint_lang_wgsl_ast_transform.lib
                            tint_lang_wgsl_common.lib
                            tint_lang_wgsl_features.lib
                            tint_lang_wgsl_helpers.lib
                            tint_lang_wgsl_inspector.lib
                            tint_lang_wgsl_intrinsic.lib
                            tint_lang_wgsl_ir.lib
                            tint_lang_wgsl_program.lib
                            tint_lang_wgsl_reader.lib
                            tint_lang_wgsl_reader_lower.lib
                            tint_lang_wgsl_reader_parser.lib
                            tint_lang_wgsl_reader_program_to_ir.lib
                            tint_lang_wgsl_resolver.lib
                            tint_lang_wgsl_sem.lib
                            tint_lang_wgsl_writer.lib
                            tint_lang_wgsl_writer_ast_printer.lib
                            tint_lang_wgsl_writer_ir_to_program.lib
                            tint_lang_wgsl_writer_raise.lib
                            tint_lang_wgsl_writer_syntax_tree_printer.lib
                            tint_utils_containers.lib
                            tint_utils_debug.lib
                            tint_utils_diagnostic.lib
                            tint_utils_generator.lib
                            tint_utils_ice.lib
                            tint_utils_id.lib
                            tint_utils_macros.lib
                            tint_utils_math.lib
                            tint_utils_memory.lib
                            tint_utils_reflection.lib
                            tint_utils_result.lib
                            tint_utils_rtti.lib
                            tint_utils_strconv.lib
                            tint_utils_symbol.lib
                            tint_utils_system.lib
                            tint_utils_text.lib
                            tint_utils_traits.lib
                            webgpu_dawn.lib
                    )
    else()
    endif()

    add_compile_definitions(RENDERER_BACKEND=3)
    add_library (RenderSys2D STATIC
                    src/RenderSys/Renderer2D.cpp
                    src/RenderSys/WebGPU/WebGPURenderer2D.cpp
    )
    add_library (RenderSys3D STATIC
                    src/RenderSys/Renderer3D.cpp
                    src/RenderSys/WebGPU/WebGPURenderer3D.cpp
                    src/RenderSys/Camera.cpp
    )
    add_library (ComputeSys STATIC
                    src/RenderSys/Compute.cpp
                    src/RenderSys/WebGPU/WebGPUCompute.cpp
    )
    
    target_link_libraries(RenderSys2D PRIVATE WEBGPU-WHOLEARCHIVE)
    target_link_libraries(RenderSys3D PRIVATE WEBGPU-WHOLEARCHIVE)
    target_link_libraries(ComputeSys PRIVATE WEBGPU-WHOLEARCHIVE)
    
endif()

target_sources(RenderSys2D 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES src/RenderSys/Renderer2D.h)
target_sources(RenderSys3D 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES src/RenderSys/Renderer3D.h src/RenderSys/RenderUtil.h)
target_sources(ComputeSys 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES src/RenderSys/Compute.h src/RenderSys/Buffer.h)

target_link_libraries(RenderSys2D PRIVATE walnut::walnut)
target_link_libraries(RenderSys3D PRIVATE walnut::walnut)
target_link_libraries(ComputeSys PRIVATE walnut::walnut)

install(TARGETS RenderSys2D
    FILE_SET renderSysFileSet
    COMPONENT Renderer2D
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)

install(TARGETS RenderSys3D
    FILE_SET renderSysFileSet
    COMPONENT Renderer3D
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include
)

install(TARGETS ComputeSys
    FILE_SET renderSysFileSet
    COMPONENT Compute
    LIBRARY DESTINATION lib
    PRIVATE_HEADER DESTINATION include
)

add_subdirectory(example)