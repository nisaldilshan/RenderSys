add_executable(FirstAnimation 
            AnimMain.cpp
)

target_link_libraries(FirstAnimation PRIVATE RenderSys3D RenderSysCommon walnut::walnut)

target_compile_definitions(FirstAnimation PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(FirstAnimation PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/example/Resources"
)
