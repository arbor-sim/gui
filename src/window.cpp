#include "window.hpp"

#include <glbinding/gl/gl.h>
#include <glbinding/glbinding.h>

#include <filesystem>

#include "ImGuizmo.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <IconsForkAwesome.h>

static void glfw_error_callback(int error, const char* description) {
    log_error("Glfw error {}:\n{}", error, description);
}

float delta_zoom    = 0.0f;
glm::vec2 mouse     = {0.0f, 0.0f};

static void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {
    delta_zoom -= (float) yoffset;
}

static void mouse_callback(GLFWwindow* window, double x, double y) {
    mouse = {x, y};
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if ((key == GLFW_KEY_MINUS) && (action == GLFW_PRESS)) delta_zoom  =  2.0f;
    if ((key == GLFW_KEY_EQUAL) && (action == GLFW_PRESS)) delta_zoom  = -2.0f;
}


bool Window::visible() { return glfwGetWindowAttrib(handle, GLFW_FOCUSED); }

Window::Window() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) log_fatal("Failed to initialise GLFW");
    log_info("Setting up window with OpenGL 4.1 Core forward compatible GLSL v410");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
    glsl_version = "#version 410";
    handle = glfwCreateWindow(1280, 720, "arbor-gui", NULL, NULL);
    if (handle == nullptr) log_fatal("Failed to obtain window");
    log_info("OpenGL version instantiated {}.{}",
             glfwGetWindowAttrib(handle, GLFW_CONTEXT_VERSION_MAJOR),
             glfwGetWindowAttrib(handle, GLFW_CONTEXT_VERSION_MINOR));
    glfwMakeContextCurrent(handle);
    glbinding::initialize(glfwGetProcAddress);
    glfwSwapInterval(1); // Enable vsync
    glfwSetScrollCallback(handle, scroll_callback);
    glfwSetCursorPosCallback(handle, mouse_callback);
    glfwSetKeyCallback(handle, key_callback);
    // if (gl3wInit()) log_fatal("Failed to initialize OpenGL loader");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Enable keyboard navigation
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    // Update key mapping
    {
        #define MAP_KEY(NAV_NO, KEY_NO) { if (io.KeysDown[KEY_NO]) io.NavInputs[NAV_NO] = 1.0f; }
        MAP_KEY(ImGuiNavInput_Activate, GLFW_KEY_SPACE);
        #undef MAP_KEY
    }
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)) log_error("ImGui docking disabled");

    // ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    set_style_dark();

    ini_file = get_resource_path("imgui.ini");
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
        auto path = get_resource_path("fonts/iosevka/iosevka-medium.ttf");
        font = io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f, nullptr, (ImWchar*) base_range.Data);
    }
    {
        static const ImWchar icons_ranges[] = { ICON_MIN_FK, ICON_MAX_FK, 0 };
        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        auto path = get_resource_path(std::filesystem::path{"fonts/icons"} / FONT_ICON_FILE_NAME_FK);
        io.Fonts->AddFontFromFileTTF(path.c_str(), 16.0f, &icons_config, icons_ranges);
    }
    log_debug("Set up fonts");

#if !__APPLE__
    log_debug("Setting icon");
    GLFWimage image;
    auto path = get_resource_path("arbor.png");
    image.pixels = stbi_load(path.c_str(), &image.width, &image.height, 0, 4); //rgba channels
    if (image.pixels != NULL) {
        try {
            glfwSetWindowIcon(handle, 1, &image);
        } catch(...) {
            log_warn("Setting icon failed, possibly on OS X?");
        }
    } else {
        log_warn("Setting icon not possible, image not found.");
    }
    stbi_image_free(image.pixels);
#endif
}

Window::~Window() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
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
    ImGuizmo::BeginFrame();
    ImGui::PushFont(font);
}

void Window::end_frame() {
    ImGui::PopFont();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(handle);
    delta_zoom = 0.0f;
}

void Window::set_style_light() {
    auto& style = ImGui::GetStyle();
    style.FrameRounding     = 2.0f;
    style.WindowPadding     = ImVec2(4.0f, 3.0f);
    style.FramePadding      = ImVec2(4.0f, 4.0f);
    style.ItemSpacing       = ImVec2(4.0f, 3.0f);
    style.IndentSpacing     = 12;
    style.ScrollbarSize     = 12;
    style.GrabMinSize       = 9;
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 0.0f;
    style.PopupBorderSize   = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.GrabRounding      = 2.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding       = 0.0f;

    auto colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.15f, 0.15f, 0.15f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.89f, 0.89f, 0.89f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.93f, 0.93f, 0.93f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_TitleBg]               = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.01f, 0.01f, 0.01f, 0.63f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Button]                = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(1.00f, 0.69f, 0.07f, 0.69f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Header]                = ImVec4(0.67f, 0.67f, 0.67f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Separator]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(1.00f, 1.00f, 1.00f, 0.18f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.16f, 0.16f, 0.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(1.00f, 0.82f, 0.46f, 0.69f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(1.00f, 0.69f, 0.07f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.87f, 0.87f, 0.87f, 1.00f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

void Window::set_style_dark() {
    auto& style = ImGui::GetStyle();
    style.FrameRounding     = 2.0f;
    style.WindowPadding     = ImVec2(4.0f, 3.0f);
    style.FramePadding      = ImVec2(4.0f, 4.0f);
    style.ItemSpacing       = ImVec2(4.0f, 3.0f);
    style.IndentSpacing     = 12;
    style.ScrollbarSize     = 12;
    style.GrabMinSize       = 9;
    style.WindowBorderSize  = 0.0f;
    style.ChildBorderSize   = 0.0f;
    style.PopupBorderSize   = 0.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
    style.WindowRounding    = 0.0f;
    style.ChildRounding     = 0.0f;
    style.FrameRounding     = 0.0f;
    style.PopupRounding     = 0.0f;
    style.GrabRounding      = 2.0f;
    style.ScrollbarRounding = 12.0f;
    style.TabRounding       = 0.0f;

    auto colors = style.Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    colors[ImGuiCol_WindowBg]              = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ChildBg]               = ImVec4(0.17f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_PopupBg]               = ImVec4(0.22f, 0.24f, 0.25f, 1.00f);
    colors[ImGuiCol_Border]                = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_BorderShadow]          = ImVec4(0.16f, 0.17f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBg]               = ImVec4(0.14f, 0.15f, 0.16f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.84f, 0.34f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBgActive]         = ImVec4(0.59f, 0.24f, 0.12f, 1.00f);
    colors[ImGuiCol_TitleBg]               = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgActive]         = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_MenuBarBg]             = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.75f, 0.30f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]             = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
    colors[ImGuiCol_SliderGrab]            = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
    colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_Button]                = ImVec4(0.19f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ButtonHovered]         = ImVec4(0.84f, 0.34f, 0.17f, 1.00f);
    colors[ImGuiCol_ButtonActive]          = ImVec4(0.59f, 0.24f, 0.12f, 1.00f);
    colors[ImGuiCol_Header]                = ImVec4(0.22f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_HeaderHovered]         = ImVec4(0.84f, 0.34f, 0.17f, 1.00f);
    colors[ImGuiCol_HeaderActive]          = ImVec4(0.59f, 0.24f, 0.12f, 1.00f);
    colors[ImGuiCol_Separator]             = ImVec4(0.17f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.75f, 0.30f, 0.15f, 1.00f);
    colors[ImGuiCol_SeparatorActive]       = ImVec4(0.59f, 0.24f, 0.12f, 1.00f);
    colors[ImGuiCol_ResizeGrip]            = ImVec4(0.84f, 0.34f, 0.17f, 0.14f);
    colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.84f, 0.34f, 0.17f, 1.00f);
    colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.59f, 0.24f, 0.12f, 1.00f);
    colors[ImGuiCol_Tab]                   = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    colors[ImGuiCol_TabHovered]            = ImVec4(0.84f, 0.34f, 0.17f, 1.00f);
    colors[ImGuiCol_TabActive]             = ImVec4(0.68f, 0.28f, 0.14f, 1.00f);
    colors[ImGuiCol_TabUnfocused]          = ImVec4(0.13f, 0.14f, 0.16f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]    = ImVec4(0.17f, 0.18f, 0.20f, 1.00f);
    colors[ImGuiCol_DockingPreview]        = ImVec4(0.19f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_DockingEmptyBg]        = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotLines]             = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.75f, 0.30f, 0.15f, 1.00f);
    colors[ImGuiCol_DragDropTarget]        = ImVec4(0.75f, 0.30f, 0.15f, 1.00f);
    colors[ImGuiCol_NavHighlight]          = ImVec4(0.75f, 0.30f, 0.15f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}
