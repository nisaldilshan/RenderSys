add_executable(BasicShading 
            main.cpp
)

target_link_libraries(BasicShading PRIVATE RenderSys3D walnut::walnut tinyobjloader)

target_compile_definitions(BasicShading PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources/Meshes"
)


