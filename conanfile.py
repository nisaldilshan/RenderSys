import os
from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.env import VirtualRunEnv
from conan.tools.scm import Git
from conan.tools.files import copy
from conan.errors import ConanInvalidConfiguration, ConanException

class RenderSysConan(ConanFile):
    name = "RenderSys"
    # source() clones the tag v<version>; tag the release when this changes
    version = "0.0.3"
    license = "MIT license"
    url = "https://github.com/nisaldilshan/RenderSys"
    description = "A simple rendering system written in C++"
    author = "Nisal Dilshan"
    settings = "os", "compiler", "build_type", "arch"
    
    options = {
        "shared": [True, False],
        "rendering_backend": ["OpenGL", "Vulkan", "WebGPU"],
        "build_examples": [True, False],
        "with_walnut": [True, False],
        "fPIC": [True, False],
        "branch": ["ANY"],
    }
    default_options = {
        "shared": False,
        "rendering_backend": "Vulkan",
        "build_examples": True,
        "with_walnut": True,
        "fPIC": True,
        "branch": "main",
    }

    def requirements(self):
        # transitive_headers: RenderSys's installed headers include these, so consumers need them too
        # The Vulkan core runs without Walnut; the WebGPU backend still gets its device from it
        if self.options.with_walnut or self.options.rendering_backend != "Vulkan":
            self.requires("walnut/2.2.0", transitive_headers=True)
        self.requires("tinyobjloader/2.0.0-rc10")
        self.requires("tinygltf/2.9.0")
        self.requires("shaderc/2023.6")
        self.requires("entt/3.14.0", transitive_headers=True)
        self.requires("glm/1.0.1", transitive_headers=True)
        if self.options.rendering_backend == "Vulkan":
            # Pin the headers to the loader's version before VMA's [~1] range can pick a newer one
            self.requires("vulkan-headers/1.3.239.0", transitive_headers=True)
            self.requires("vulkan-loader/1.3.239.0", transitive_headers=True)
            self.requires("vulkan-memory-allocator/3.3.0")
        elif self.options.rendering_backend == "WebGPU":
            pass # self.requires("webgpu/latest")
        else:
            raise ConanInvalidConfiguration(f"Unsupported Renderer Type: {self.options.rendering_backend}")

    def build_requirements(self):
        self.tool_requires("cmake/[>=3.25.3]")
    
    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.get_safe("shared"):
            self.options.rm_safe("fPIC")
        self.options["walnut"].rendering_backend = self.options.rendering_backend

    def source(self):
        git = Git(self)
        target_ref = "main" if self.version == "latest" else f"v{self.version}"
        self.output.info(f"Attempting to checkout git ref: {target_ref}")

        try:
            self.run(f"git clone --depth 1 --branch {target_ref} {self.url}.git .")
        except Exception as e:
            raise ConanException(
                f"\n\nERROR: Could not checkout version '{self.version}'.\n"
                f"Attempted to fetch git ref '{target_ref}' from {self.url}\n"
                f"Please verify that the tag 'v{self.version}' exists in the remote repository.\n"
                f"Original Git Error: {str(e)}\n"
            )

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["RENDERER"] = self.options.rendering_backend
        # The examples run inside Walnut, so a Walnut-free build skips them
        build_examples = bool(self.options.build_examples) and bool(self.options.with_walnut)
        if self.options.build_examples and not build_examples:
            self.output.info("with_walnut=False: not building the examples")
        tc.variables["BUILD_EXAMPLES"] = build_examples
        tc.variables["RENDERSYS_WITH_WALNUT"] = self.options.with_walnut
        self.output.info(f"Renderer: {self.options.rendering_backend}")
        tc.generate()
        
        deps = CMakeDeps(self)
        deps.generate()

        # Walnut's run environment points VK_LAYER_PATH at a loader-compatible validation layer manifest.
        # The VS Code debugger cannot source conanrun.sh, so hand the variables over as an envFile
        # (see .vscode/launch.json). Every launch config reads it, so it is written for every option
        # combination, empty when there is no layer to enable.
        layer_path = None
        if self.options.with_walnut and self.options.rendering_backend == "Vulkan":
            layer_path = VirtualRunEnv(self).environment().vars(self, scope="run").get("VK_LAYER_PATH")
        with open(os.path.join(self.build_folder, "vk_layer_path.env"), "w") as env_file:
            if layer_path:
                env_file.write(f"VK_LAYER_PATH={layer_path}\n")
                env_file.write("VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation\n")

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        copy(self, pattern="*", 
             src=os.path.join(self.source_folder, "src", "resources"), 
             dst=os.path.join(self.package_folder, "res"))

    def package_info(self):
        # Every component lists its external packages too: the libraries are static, so consumers
        # link their private dependencies, and Conan checks that each direct requirement is used.
        # The WebGPU backend still takes its device from Walnut.
        backend_deps = ['walnut::walnut'] if self.options.rendering_backend != "Vulkan" else []

        # Component: Renderer2D
        self.cpp_info.components['Renderer2D'].set_property('cmake_target_name', 'RenderSys::Renderer2D')
        self.cpp_info.components['Renderer2D'].libs = ['RenderSys2D']
        self.cpp_info.components['Renderer2D'].requires = ['Common', 'tinyobjloader::tinyobjloader',
                                                           'shaderc::shaderc', 'glm::glm'] + backend_deps

        # Component: Renderer3D
        self.cpp_info.components['Renderer3D'].set_property('cmake_target_name', 'RenderSys::Renderer3D')
        self.cpp_info.components['Renderer3D'].libs = ['RenderSys3D']
        self.cpp_info.components['Renderer3D'].requires = ['Common', 'tinyobjloader::tinyobjloader',
                                                           'tinygltf::tinygltf', 'shaderc::shaderc',
                                                           'entt::entt', 'glm::glm'] + backend_deps

        # Component: Compute
        self.cpp_info.components['Compute'].set_property('cmake_target_name', 'RenderSys::Compute')
        self.cpp_info.components['Compute'].libs = ['ComputeSys']
        self.cpp_info.components['Compute'].requires = ['Common', 'shaderc::shaderc', 'glm::glm'] + backend_deps

        # Component: Common
        self.cpp_info.components['Common'].set_property('cmake_target_name', 'RenderSys::Common')
        self.cpp_info.components['Common'].libs = ['RenderSysCommon']
        self.cpp_info.components['Common'].resdirs = ['res']
        backend = {"OpenGL": 1, "Vulkan": 2, "WebGPU": 3}[str(self.options.rendering_backend)]
        self.cpp_info.components['Common'].defines = [f"RENDERSYS_BACKEND={backend}"]
        if self.options.rendering_backend == "Vulkan":
            self.cpp_info.components['Common'].requires = ['vulkan-headers::vulkan-headers',
                                                           'vulkan-loader::vulkan-loader',
                                                           'vulkan-memory-allocator::vulkan-memory-allocator']
        else:
            self.cpp_info.components['Common'].requires = ['glm::glm'] + backend_deps

        if self.options.with_walnut:
            # Component: Walnut (runs RenderSys inside a Walnut::Application)
            self.cpp_info.components['Walnut'].set_property('cmake_target_name', 'RenderSys::Walnut')
            self.cpp_info.components['Walnut'].libs = ['RenderSysWalnut']
            self.cpp_info.components['Walnut'].requires = ['Common', 'walnut::walnut']

            # Component: Editor (ImGui panels for the 3D scene)
            self.cpp_info.components['Editor'].set_property('cmake_target_name', 'RenderSys::Editor')
            self.cpp_info.components['Editor'].libs = ['RenderSysEditor']
            self.cpp_info.components['Editor'].requires = ['Renderer3D', 'walnut::walnut']

    def layout(self):
        # Standardizes build and source folder structures for Conan 2.x
        cmake_layout(self)