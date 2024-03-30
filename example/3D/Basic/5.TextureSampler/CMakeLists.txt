add_executable(TextureSampler 
            main.cpp
)

target_link_libraries(TextureSampler PRIVATE RenderSys3D walnut::walnut tinyobjloader)

target_compile_definitions(TextureSampler PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


