#include "window.hpp"

#include <filesystem>

#include <IconsForkAwesome.h>

static void glfw_error_callback(int error, const char* description) {
    log_error("Glfw error {}:\n{}", error, description);
}

float delta_phi     = 0.0f;
float delta_zoom    = 0.0f;
glm::vec2 delta_pos = {0.0f, 0.0f};

float mouse_x;
float mouse_y;

static void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {
    delta_zoom -= (float) yoffset;
}

static void mouse_callback(GLFWwindow* window, double x, double y) {

    auto lb_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    auto alt_key = (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS);
    auto ctrl_key = (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

    auto eps = 0.1;
    auto dx = mouse_x - x; mouse_x = x;
    auto dy = mouse_y - y; mouse_y = y;

    if (lb_down && ctrl_key) {
        delta_pos.x = 2.0f*((dx > eps) - (dx < eps));
        delta_pos.y = 2.0f*((dy > eps) - (dy < eps));
    }

    if (lb_down && !ctrl_key) {
        delta_phi = ((dx > eps) - (dx < eps))*0.1f;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_DOWN) &&  (action == GLFW_PRESS)) delta_pos.y =  15.0f;
    if ((key == GLFW_KEY_UP)   &&  (action == GLFW_PRESS)) delta_pos.y = -15.0f;
    if ((key == GLFW_KEY_LEFT) &&  (action == GLFW_PRESS)) delta_pos.x =  15.0f;
    if ((key == GLFW_KEY_RIGHT) && (action == GLFW_PRESS)) delta_pos.x = -15.0f;
    if ((key == GLFW_KEY_MINUS) && (action == GLFW_PRESS)) delta_zoom  =  2.0f;
    if ((key == GLFW_KEY_EQUAL) && (action == GLFW_PRESS)) delta_zoom  = -2.0f;
}


bool Window::visible() { return glfwGetWindowAttrib(handle, GLFW_FOCUSED); }

Window::Window() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) log_fatal("Failed to initialise GLFW");
#if __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
    glsl_version = "#version 150";
    log_info("Set up on Apple: OpenGL 3.2 Core forward compatible GLSL v150");
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
    glsl_version = "#version 130";
    log_info("Set up on Linux: OpenGL 3.0 GLSL v130");
#endif
    // glEnable(GL_MULTISAMPLE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    // Create window with graphics context
    handle = glfwCreateWindow(1280, 720, "arbor-gui", NULL, NULL);
    if (handle == nullptr) log_fatal("Failed to obtain window");
    glfwMakeContextCurrent(handle);
    glfwSwapInterval(1); // Enable vsync
    glfwSetScrollCallback(handle, scroll_callback);
    glfwSetCursorPosCallback(handle, mouse_callback);
    glfwSetKeyCallback(handle, key_callback);
    if (gl3wInit()) log_fatal("Failed to initialize OpenGL loader");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)) log_error("ImGui docking disabled");

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    std::filesystem::path base = ARBORGUI_RESOURCES_BASE;
    ini_file = base / "imgui.ini";
    io.IniFilename = ini_file.c_str();
    log_debug("Set Dear ImGUI ini file: {}", ini_file);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(handle, true);
    ImGui_ImplOpenGL3_Init(glsl_version.c_str());

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    {
        static ImVector<ImWchar> base_range;
        ImFontGlyphRangesBuilder builder;
        builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
        auto text = u8"ΔΩ·²";
        builder.AddText((const char*) text);
        builder.BuildRanges(&base_range);
        auto path = base / "fonts/iosevka/iosevka-medium.ttf";
        font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f, nullptr, (ImWchar*) base_range.Data);
    }
    {
        static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        auto path = base / "fonts/icons" / FONT_ICON_FILE_NAME_FK;
        io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f, &icons_config, icons_ranges);
    }
    log_debug("Set up fonts");
}

Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(handle);
    glfwTerminate();
}

bool Window::running() { return !glfwWindowShouldClose(handle); }

void Window::begin_frame() {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::PushFont(font);
}

void Window::end_frame() {
    ImGui::PopFont();
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(handle);
    delta_pos = {0.0f, 0.0f};
    delta_phi = 0.0f;
    delta_zoom = 0.0f;
}
