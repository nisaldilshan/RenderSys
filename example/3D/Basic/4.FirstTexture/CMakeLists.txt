add_executable(FirstTexture 
            main.cpp
)

target_link_libraries(FirstTexture PRIVATE RenderSys3D RenderSysCommon walnut::walnut)

target_compile_definitions(FirstTexture PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)


