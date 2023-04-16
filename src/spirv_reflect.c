/*cxe{
    -std=c99
    -Wall -Werror
    -fsanitize=address
    -I$VULKAN_SDK/Include
    -I$VULKAN_SDK/Source
    -if (--target=[darwin]) {
        -pre { mkdir -p ../lib/macos }
        -o ../lib/macos/libspirv_reflect.a
        -Wno-deprecated-declarations
    }
    -if (--target=[windows]) {
        -pre { mkdir -p ../lib/windows }
        -o ../lib/windows/spirv_reflect.lib
        -D_CRT_SECURE_NO_WARNINGS
    }
    -c // static library
}*/

#include <SPIRV-Reflect/spirv_reflect.c>