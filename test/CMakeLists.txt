add_executable(HeadlessRender HeadlessRender/main.cpp)
target_link_libraries(HeadlessRender PRIVATE RenderSys3D)
add_test(NAME HeadlessRender COMMAND HeadlessRender)
