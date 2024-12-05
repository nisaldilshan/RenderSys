add_executable(TextureLoader
            main.cpp
)

target_link_libraries(TextureLoader PRIVATE RenderSys3D walnut::walnut stb::stb)

target_compile_definitions(TextureLoader PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


