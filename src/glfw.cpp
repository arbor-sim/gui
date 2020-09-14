#include <GLFW/glfw3.h>

float phi  = 0.0f;
float zoom = 45.0f;

static void glfw_error_callback(int error, const char* description) {
    log_error("Glfw error:\n{}", description);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    phi += 0.1 * (float) xoffset;
    if (phi > 2.0f*PI) phi -= 2*PI;
    if (phi <    0.0f) phi += 2*PI;
    zoom -= (float) yoffset;
    if (zoom < 1.0f)  zoom =  1.0f;
    if (zoom > 45.0f) zoom = 45.0f;
}


struct Window {
    Window() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) log_fatal("Failed to initialise GLFW");
#if __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
#else
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
#endif
        // Create window with graphics context
        handle = glfwCreateWindow(1280, 720, "arbor-gui", NULL, NULL);
        if (handle == nullptr) log_fatal("Failed to obtain window");
        glfwMakeContextCurrent(handle);
        glfwSwapInterval(1); // Enable vsync
        glfwSetScrollCallback(handle, scroll_callback);

    }

    ~Window() {
        glfwDestroyWindow(handle);
        glfwTerminate();
    }

    bool running() { return !glfwWindowShouldClose(handle); }

    std::pair<int, int> get_extent()  {
        int w, h;
        glfwGetFramebufferSize(handle, &w, &h);
        return {w, h};
    }

    void swap_buffers() { glfwSwapBuffers(handle); }

    void poll() {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
    }

    GLFWwindow* handle = nullptr;
#if __APPLE__
        // GL 3.2 + GLSL 150
        const char* glsl_version = "#version 150";
#else
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
#endif
};
