cmake_minimum_required (VERSION 3.25.0)
project (RenderSys)

find_package(walnut REQUIRED)
find_package(tinyobjloader REQUIRED)
find_package(shaderc REQUIRED)
find_package(TinyGLTF REQUIRED)
find_package(EnTT REQUIRED)
find_package(glm REQUIRED)
include(FetchContent)

add_library (RenderSys2D STATIC
                src/RenderSys/Renderer2D.cpp
                src/RenderSys/Shader.cpp
                src/RenderSys/Geometry.cpp)

add_library (RenderSys3D STATIC
                src/RenderSys/Renderer3D.cpp
                src/RenderSys/Shader.cpp
                src/RenderSys/Camera/PerspectiveCamera.cpp
                src/RenderSys/Camera/EditorCameraController.cpp
                src/RenderSys/Geometry.cpp
                src/RenderSys/Buffer.cpp
                src/RenderSys/Texture.cpp
                src/RenderSys/Material.cpp
                src/RenderSys/Resource.cpp
                src/RenderSys/InstanceBuffer.cpp
                src/RenderSys/Scene/GLTFModel.cpp
                src/RenderSys/Scene/Model.cpp
                src/RenderSys/Scene/Scene.cpp
                src/RenderSys/Scene/SceneGraph.cpp
                src/RenderSys/Scene/SceneHierarchyPanel.cpp
                src/RenderSys/Scene/Skeleton.cpp
                src/RenderSys/Scene/Animation.cpp
                src/RenderSys/Scene/UUID.cpp
                src/RenderSys/Components/TransformComponent.cpp
                src/RenderSys/Components/MeshComponent.cpp
                src/RenderSys/Components/TagAndIDComponents.cpp
            )

add_library (ComputeSys STATIC
                src/RenderSys/Compute.cpp
                src/RenderSys/Shader.cpp)

target_include_directories(RenderSys2D PRIVATE src ${imgui_INCLUDE_DIRS}/../res/bindings)
target_include_directories(RenderSys3D PRIVATE src ${imgui_INCLUDE_DIRS}/../res/bindings)
target_include_directories(ComputeSys PRIVATE src ${imgui_INCLUDE_DIRS}/../res/bindings)

message(STATUS "RENDERER: " ${RENDERER})
if(RENDERER STREQUAL "Vulkan")
    find_package(VulkanMemoryAllocator REQUIRED)
    target_sources(RenderSys2D PRIVATE
                    src/RenderSys/Vulkan/VulkanRenderer2D.cpp
                    src/RenderSys/Vulkan/VulkanRendererUtils.cpp
    )
    target_sources(RenderSys3D PRIVATE
                    src/RenderSys/Vulkan/VulkanRenderer3D.cpp
                    src/RenderSys/Vulkan/VulkanRendererUtils.cpp
                    src/RenderSys/Vulkan/VulkanBuffer.cpp
                    src/RenderSys/Vulkan/VulkanTexture.cpp
                    src/RenderSys/Vulkan/VulkanMaterial.cpp
                    src/RenderSys/Vulkan/VulkanResource.cpp
                    src/RenderSys/Vulkan/VulkanShadowMap.cpp
                    src/RenderSys/Vulkan/Pipeline/VulkanPipeline.cpp
                    src/RenderSys/Vulkan/Pipeline/VulkanPbrRenderPipeline.cpp
                    src/RenderSys/Vulkan/Pipeline/VulkanShadowRenderPipeline.cpp
    )
    target_sources(ComputeSys PRIVATE
                    src/RenderSys/Vulkan/VulkanCompute.cpp
                    src/RenderSys/Vulkan/VulkanRendererUtils.cpp
    )

    target_link_libraries(RenderSys2D PRIVATE GPUOpen::VulkanMemoryAllocator)
    target_link_libraries(RenderSys3D PRIVATE GPUOpen::VulkanMemoryAllocator)
    target_link_libraries(ComputeSys PRIVATE GPUOpen::VulkanMemoryAllocator)
    target_link_libraries(RenderSys2D PUBLIC glm::glm)
    target_link_libraries(RenderSys3D PUBLIC glm::glm)
    target_link_libraries(ComputeSys PUBLIC glm::glm)
else()
    set(WEBGPU_BACKEND DAWN)
    FetchContent_Declare(
        WebGPU-distribution
        GIT_REPOSITORY https://github.com/eliemichel/WebGPU-distribution.git
        GIT_TAG        v0.3.0-gamma
        QUIET
        SOURCE_DIR     ${CMAKE_SOURCE_DIR}/external_deps/WebGPU-distribution-src
        BINARY_DIR     ${CMAKE_SOURCE_DIR}/external_deps/WebGPU-distribution-build
        SUBBUILD_DIR   ${CMAKE_SOURCE_DIR}/external_deps/WebGPU-distribution-subbuild
    )
    FetchContent_MakeAvailable(WebGPU-distribution)

    target_sources(RenderSys2D PRIVATE
                    src/RenderSys/WebGPU/WebGPURenderer2D.cpp)
    target_sources(RenderSys3D PRIVATE
                    src/RenderSys/WebGPU/WebGPURenderer3D.cpp
                    src/RenderSys/WebGPU/WebGPUTexture.cpp
                    src/RenderSys/WebGPU/WebGPUBuffer.cpp)
    target_sources(ComputeSys PRIVATE
                    src/RenderSys/WebGPU/WebGPUCompute.cpp)
    
    target_link_libraries(RenderSys2D PRIVATE webgpu)
    target_link_libraries(RenderSys3D PRIVATE webgpu)
    target_link_libraries(ComputeSys PRIVATE webgpu)
    
endif()

target_sources(RenderSys2D 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES src/RenderSys/Renderer2D.h src/RenderSys/Shader.h src/RenderSys/Geometry.h)
target_sources(RenderSys3D 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES   src/RenderSys/Renderer3D.h 
                        src/RenderSys/RenderUtil.h 
                        src/RenderSys/Texture.h 
                        src/RenderSys/TextureSampler.h 
                        src/RenderSys/Camera/ICamera.h
                        src/RenderSys/Camera/PerspectiveCamera.h
                        src/RenderSys/Camera/EditorCameraController.h
                        src/RenderSys/Material.h
                        src/RenderSys/Resource.h
                        src/RenderSys/Scene/Mesh.h
                        src/RenderSys/Scene/Model.h
                        src/RenderSys/Scene/Scene.h
                        src/RenderSys/Scene/SceneHierarchyPanel.h
                        src/RenderSys/Scene/Skeleton.h
                        src/RenderSys/Scene/Animation.h
                        src/RenderSys/Scene/UUID.h
                        src/RenderSys/Scene/SceneGraph.h
                        src/RenderSys/Components/TransformComponent.h
                        src/RenderSys/Components/MeshComponent.h
                        src/RenderSys/Components/TagAndIDComponents.h
                        src/RenderSys/InstanceBuffer.h
                )
target_sources(ComputeSys 
                PUBLIC FILE_SET renderSysFileSet 
                TYPE HEADERS 
                BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                FILES src/RenderSys/Compute.h src/RenderSys/Buffer.h)

target_link_libraries(RenderSys2D PRIVATE walnut::walnut tinyobjloader::tinyobjloader shaderc::shaderc)
target_link_libraries(RenderSys3D PRIVATE walnut::walnut tinyobjloader::tinyobjloader shaderc::shaderc TinyGLTF::TinyGLTF)
target_link_libraries(RenderSys3D PUBLIC EnTT::EnTT)
target_link_libraries(ComputeSys PRIVATE walnut::walnut shaderc::shaderc)

install(TARGETS RenderSys2D
    FILE_SET renderSysFileSet
    COMPONENT Renderer2D
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include)

install(TARGETS RenderSys3D
    FILE_SET renderSysFileSet
    COMPONENT Renderer3D
    LIBRARY DESTINATION lib
    INCLUDES DESTINATION include)

install(TARGETS ComputeSys
    FILE_SET renderSysFileSet
    COMPONENT Compute
    LIBRARY DESTINATION lib
    PRIVATE_HEADER DESTINATION include)

add_subdirectory(src/RenderSys/Common)

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()