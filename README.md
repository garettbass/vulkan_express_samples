# Vulkan Express Samples

These are simple Vulkan API samples using Vulkan Express (https://github.com/garettbass/vulkan_express) to simplify some aspects of setup and per-frame resource management.

## Getting Started

These samples can be compiled any way you please, but my development workflow is based on `bash` and `cxe` (https://github.com/garettbass/cxe)

```sh
# clone this repository,
# including glfw and vulkan_express submodules
$ git clone --recurse-submodules https://github.com/garettbass/vulkan_express_samples

# change to vulkan_express_samples directory
$ cd vulkan_express_samples

# compile cxe for your platform
# add cxe and Vulkan SDK to PATH
# NOTE: this does not modify your global PATH,
# only this terminal session
$ source devenv.sh

# compile and run sample01_clearSwapchainImage.c
$ cxe src/sample01_clearSwapchainImage.c --

# compile and run sample02_helloTriangle.c
$ cxe src/sample02_helloTriangle.c --
```