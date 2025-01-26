from conan import ConanFile
from conan.tools.scm import Git
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
from conan.tools.build import check_min_cppstd
from conan.tools.layout import basic_layout
from conan.tools.files import copy
from conan.tools.scm import Version
import os

required_conan_version = ">=1.52.0"

class VulkanMemoryAllocatorConan(ConanFile):
    name = "vulkan-memory-allocator"
    version = "3.1.0"
    license = "MIT"
    homepage = "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator"
    url = "https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator"
    description = "Easy to integrate Vulkan memory allocation library."
    topics = ("vulkan", "memory-allocator", "graphics")
    package_type = "header-library"
    settings = "os", "arch", "compiler", "build_type"

    @property
    def _min_cppstd(self):
        return "11" if Version(self.version) < "3.0.0" else "14"

    def layout(self):
        basic_layout(self, src_folder="src")

    def requirements(self):
        self.requires("vulkan-headers/1.3.239.0")

    def package_id(self):
        self.info.clear()

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, self._min_cppstd)

    def source(self):
        git = Git(self)
        clone_args = ['--depth', '1', '--branch', "v" + self.version]
        git.clone(url=self.url + ".git", args=clone_args)

    def generate(self):
        deps = CMakeDeps(self)
        deps.check_components_exist = True
        deps.generate()

    def package(self):
        self.copy(pattern="*/include/vk_mem_alloc.h", dst="include", keep_path=False)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []