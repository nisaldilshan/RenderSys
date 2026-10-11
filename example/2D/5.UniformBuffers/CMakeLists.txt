add_executable(UniformBuffers 
            main.cpp
)

target_link_libraries(UniformBuffers PRIVATE RenderSys2D RenderSysWalnut)

target_compile_definitions(UniformBuffers PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)


