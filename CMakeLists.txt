cmake_minimum_required (VERSION 3.25.0)
project (RenderSys)

# The Walnut adapter lets RenderSys run inside a Walnut::Application; the examples need it.
option(RENDERSYS_WITH_WALNUT "Build the RenderSysWalnut and RenderSysEditor adapter libraries" ON)
if(BUILD_EXAMPLES AND NOT RENDERSYS_WITH_WALNUT)
    message(FATAL_ERROR "BUILD_EXAMPLES requires RENDERSYS_WITH_WALNUT")
endif()
if(RENDERSYS_WITH_WALNUT OR NOT RENDERER STREQUAL "Vulkan")
    find_package(walnut REQUIRED)
endif()
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

target_include_directories(RenderSys2D PRIVATE src)
target_include_directories(RenderSys3D PRIVATE src)
target_include_directories(ComputeSys PRIVATE src)

# The names Conan consumers link, so in-tree builds can use them too
add_library(RenderSys::Renderer2D ALIAS RenderSys2D)
add_library(RenderSys::Renderer3D ALIAS RenderSys3D)
add_library(RenderSys::Compute ALIAS ComputeSys)

message(STATUS "RENDERER: " ${RENDERER})
# Backend switch for the public headers: 1 = OpenGL, 2 = Vulkan, 3 = WebGPU
if(RENDERER STREQUAL "Vulkan")
    set(RENDERSYS_BACKEND 2)
else()
    set(RENDERSYS_BACKEND 3)
endif()
target_compile_definitions(RenderSys2D PUBLIC RENDERSYS_BACKEND=${RENDERSYS_BACKEND})
target_compile_definitions(RenderSys3D PUBLIC RENDERSYS_BACKEND=${RENDERSYS_BACKEND})
target_compile_definitions(ComputeSys PUBLIC RENDERSYS_BACKEND=${RENDERSYS_BACKEND})

if(RENDERER STREQUAL "Vulkan")
    find_package(VulkanMemoryAllocator REQUIRED)
    find_package(VulkanLoader REQUIRED)
    target_sources(RenderSys2D PRIVATE
                    src/RenderSys/Vulkan/VulkanRenderer2D.cpp
                    src/RenderSys/Vulkan/VulkanRendererUtils.cpp
                    src/RenderSys/Vulkan/Pipeline/VulkanPipeline.cpp
                    src/RenderSys/Vulkan/Pipeline/VulkanRender2DPipeline.cpp
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
    # RenderSysCommon owns the Vulkan context and the single VMA implementation
    target_link_libraries(RenderSys2D PUBLIC RenderSysCommon)
    target_link_libraries(RenderSys3D PUBLIC RenderSysCommon)
    target_link_libraries(ComputeSys PUBLIC RenderSysCommon)
    target_link_libraries(RenderSys2D PUBLIC glm::glm)
    target_link_libraries(RenderSys3D PUBLIC glm::glm)
    target_link_libraries(ComputeSys PUBLIC glm::glm)

    if(RENDERSYS_WITH_WALNUT)
        # Walnut (transitively) requires vulkan-validationlayers; Conan's generated manifest has a
        # 4-part api_version the loader's parser rejects, so the layer silently never loads. Rewrite
        # a loader-compatible copy at configure time and point VK_LAYER_PATH at it for debugging
        # (see .vscode/launch.json).
        file(GLOB _rendersys_vvl_data_files "${CMAKE_BINARY_DIR}/generators/vulkan-validationlayers-*-data.cmake")
        if(_rendersys_vvl_data_files)
            list(GET _rendersys_vvl_data_files 0 _rendersys_vvl_data_file)
            include(${_rendersys_vvl_data_file})
            string(TOUPPER "${CMAKE_BUILD_TYPE}" _rendersys_vvl_build_type)
            if(DEFINED vulkan-validationlayers_PACKAGE_FOLDER_${_rendersys_vvl_build_type})
                set(RENDERSYS_VULKAN_LAYER_DIR "${CMAKE_BINARY_DIR}/vulkan-layers/explicit_layer.d")
                add_custom_target(RenderSysFixVulkanValidationManifest ALL
                    COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDERSYS_VULKAN_LAYER_DIR}"
                    COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/fix_vulkan_validation_manifest.py"
                            "${vulkan-validationlayers_PACKAGE_FOLDER_${_rendersys_vvl_build_type}}"
                            "${RENDERSYS_VULKAN_LAYER_DIR}"
                    COMMENT "Generating a loader-compatible VK_LAYER_KHRONOS_validation manifest"
                    VERBATIM
                )
            endif()
            unset(_rendersys_vvl_build_type)
        endif()
        unset(_rendersys_vvl_data_files)
    endif()
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
    # The WebGPU backend still takes its device from Walnut
    target_link_libraries(RenderSys2D PRIVATE walnut::walnut)
    target_link_libraries(RenderSys3D PRIVATE walnut::walnut)
    target_link_libraries(ComputeSys PRIVATE walnut::walnut)
    
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

target_link_libraries(RenderSys2D PRIVATE tinyobjloader::tinyobjloader shaderc::shaderc)
target_link_libraries(RenderSys3D PRIVATE tinyobjloader::tinyobjloader shaderc::shaderc TinyGLTF::TinyGLTF)
target_link_libraries(RenderSys3D PUBLIC EnTT::EnTT)
target_link_libraries(ComputeSys PRIVATE shaderc::shaderc)

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

if(RENDERSYS_WITH_WALNUT)
    # Hosts RenderSys inside Walnut: adopts Walnut's device, reads Walnut's input
    add_library(RenderSysWalnut STATIC src/RenderSys/Walnut/WalnutAdapter.cpp)
    target_include_directories(RenderSysWalnut PRIVATE src)
    target_link_libraries(RenderSysWalnut PUBLIC RenderSysCommon walnut::walnut)
    target_sources(RenderSysWalnut
                    PUBLIC FILE_SET renderSysFileSet
                    TYPE HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                    FILES src/RenderSys/Walnut/WalnutAdapter.h src/RenderSys/Walnut/WalnutInput.h)

    # ImGui editor panels for the 3D scene, drawn through Walnut's ImGui
    add_library(RenderSysEditor STATIC src/RenderSys/Scene/SceneHierarchyPanel.cpp)
    target_include_directories(RenderSysEditor PRIVATE src)
    target_link_libraries(RenderSysEditor PUBLIC RenderSys3D PRIVATE walnut::walnut)
    add_library(RenderSys::Walnut ALIAS RenderSysWalnut)
    add_library(RenderSys::Editor ALIAS RenderSysEditor)
    target_sources(RenderSysEditor
                    PUBLIC FILE_SET renderSysFileSet
                    TYPE HEADERS
                    BASE_DIRS ${CMAKE_CURRENT_LIST_DIR}/src
                    FILES src/RenderSys/Scene/SceneHierarchyPanel.h)

    install(TARGETS RenderSysWalnut
        FILE_SET renderSysFileSet
        COMPONENT Walnut
        LIBRARY DESTINATION lib
        INCLUDES DESTINATION include)

    install(TARGETS RenderSysEditor
        FILE_SET renderSysFileSet
        COMPONENT Editor
        LIBRARY DESTINATION lib
        INCLUDES DESTINATION include)
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()

# Headless tests on a device RenderSys creates itself; run them through conanrun.bat for validation layers
option(RENDERSYS_BUILD_TESTS "Build the RenderSys tests" ${BUILD_EXAMPLES})
if(RENDERSYS_BUILD_TESTS AND RENDERER STREQUAL "Vulkan")
    enable_testing()
    add_subdirectory(test)
endif()