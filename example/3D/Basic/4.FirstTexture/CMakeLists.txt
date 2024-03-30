add_executable(FirstTexture 
            main.cpp
)

target_link_libraries(FirstTexture PRIVATE RenderSys3D walnut::walnut tinyobjloader)

target_compile_definitions(FirstTexture PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


