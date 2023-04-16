/*cxe{
    -std=c99
    -Wall -Werror
    -fsanitize=address
    -if (--target=[darwin]) {
        -pre { mkdir -p ../lib/macos }
        -o ../lib/macos/libglfw3.a
        -Wno-deprecated-declarations
        -x objective-c
    }
    -if (--target=[windows]) {
        -pre { mkdir -p ../lib/windows }
        -o ../lib/windows/glfw3.lib
        -D_CRT_SECURE_NO_WARNINGS
    }
    -c // static library
}*/

#if defined(_WIN32)

    #define _GLFW_WIN32
    #include "../ext/glfw/src/monitor.c"
    #include "../ext/glfw/src/init.c"
    #include "../ext/glfw/src/vulkan.c"
    #include "../ext/glfw/src/input.c"
    #include "../ext/glfw/src/context.c"
    #include "../ext/glfw/src/window.c"
    #include "../ext/glfw/src/osmesa_context.c"
    #include "../ext/glfw/src/egl_context.c"
    #include "../ext/glfw/src/wgl_context.c"
    #include "../ext/glfw/src/win32_thread.c"
    #include "../ext/glfw/src/win32_init.c"
    #include "../ext/glfw/src/win32_monitor.c"
    #include "../ext/glfw/src/win32_time.c"
    #include "../ext/glfw/src/win32_joystick.c"
    #include "../ext/glfw/src/win32_window.c"

#elif defined(__APPLE__)

    #include <TargetConditionals.h>
    #ifdef TARGET_OS_MAC

        #define _GLFW_COCOA
        #include "../ext/glfw/src/monitor.c"
        #include "../ext/glfw/src/init.c"
        #include "../ext/glfw/src/vulkan.c"
        #include "../ext/glfw/src/input.c"
        #include "../ext/glfw/src/context.c"
        #include "../ext/glfw/src/window.c"
        #include "../ext/glfw/src/osmesa_context.c"
        #include "../ext/glfw/src/egl_context.c"
        #include "../ext/glfw/src/nsgl_context.m"
        #include "../ext/glfw/src/posix_thread.c"
        #include "../ext/glfw/src/cocoa_time.c"
        #include "../ext/glfw/src/cocoa_joystick.m"
        #include "../ext/glfw/src/cocoa_init.m"
        #include "../ext/glfw/src/cocoa_window.m"
        #include "../ext/glfw/src/cocoa_monitor.m"

    #else

        #error "unsupported platform"

    #endif

#elif defined(__linux)

    // _GLFW_X11 or _GLFW_WAYLAND
    #error "unsupported platform"

#else

    #error "unsupported platform"

#endif
