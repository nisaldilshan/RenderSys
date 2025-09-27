add_executable(ShadowMapping 
            ShadowMain.cpp
)

target_link_libraries(ShadowMapping PRIVATE RenderSys3D RenderSysCommon walnut::walnut)

target_compile_definitions(ShadowMapping PRIVATE
    SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_compile_definitions(ShadowMapping PRIVATE
    RESOURCE_DIR="${CMAKE_SOURCE_DIR}/src/resources"
)
