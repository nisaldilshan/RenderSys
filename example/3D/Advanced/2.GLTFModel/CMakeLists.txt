add_executable(GLTFModel 
            main.cpp
)

target_link_libraries(GLTFModel PRIVATE RenderSys3D walnut::walnut)

target_compile_definitions(GLTFModel PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(GLTFModel PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)
