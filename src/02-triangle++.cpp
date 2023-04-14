/*cxe{
    -pre { glslc 02-triangle.vert -mfmt=c -o 02-triangle.vert.h }
    -pre { glslc 02-triangle.frag -mfmt=c -o 02-triangle.frag.h }
    -pre { $CXE glfw3.c }
    -std=c++20
    -Wall -Werror
    -Wno-unused-variable
    -Wno-unused-but-set-variable
    -ferror-limit=2
    -fsanitize=address
    -I$VULKAN_SDK/include
    -I../inc
    -if (--target=[darwin]) {
        -pre { mkdir -p ../bin/macos }
        -o../bin/macos/$CXE_SRC_NAME
        -MMD -MF ../bin/macos/$CXE_SRC_NAME.dep
        -framework Cocoa
        -framework IOKit
        -framework OpenGL
        -L../lib/macos
    }
    -if (--target=[linux]) {
        -pre { mkdir -p ../bin/linux }
        -o../bin/linux/$CXE_SRC_NAME
        -MMD -MF ../bin/linux/$CXE_SRC_NAME.dep
        -L../lib/linux
    }
    -if (--target=[windows]) {
        -pre { mkdir -p ../bin/windows }
        -o../bin/windows/$CXE_SRC_NAME.exe
        -MMD -MF ../bin/windows/$CXE_SRC_NAME.dep
        -lgdi32
        -lshell32
        -luser32
        -L../lib/windows
    }
    -if (--target=[msvc]) {
        -D_CRT_SECURE_NO_WARNINGS
    }
    -lglfw3
}*/

#ifndef SAMPLE_SOURCE
#define SAMPLE_SOURCE "02-triangle++.cpp"
#endif

#include "02-triangle.c"