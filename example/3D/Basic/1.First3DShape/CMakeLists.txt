add_executable(First3DShape 
            main.cpp
)

target_link_libraries(First3DShape PRIVATE RenderSys3D RenderSysCommon walnut::walnut)

target_compile_definitions(First3DShape PRIVATE
    RESOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)


