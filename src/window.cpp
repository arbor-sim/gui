#include "window.hpp"

#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <IconsForkAwesome.h>

static void glfw_error_callback(int error, const char* description) {
    log_error("Glfw error {}:\n{}", error, description);
}

float delta_zoom    = 0.0f;
glm::vec2 delta_pos = {0.0f, 0.0f};
glm::vec2 delta_rot = {0.0f, 0.0f};
glm::vec2 mouse     = {0.0f, 0.0f};

static void scroll_callback(GLFWwindow*, double xoffset, double yoffset) {
    delta_zoom -= (float) yoffset;
}

static void mouse_callback(GLFWwindow* window, double x, double y) {
    auto lb_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    auto mb_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    auto alt_key = (glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS);
    auto ctrl_key = (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) ||
        (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS);

    auto eps = 0.01;
    auto
        d = mouse - glm::vec2{x, y},
        a = glm::abs(d);
    mouse = {x, y};

    if (lb_down &&  ctrl_key) delta_pos = {-(a.x > eps)*d.x, (a.y > eps)*d.y};
    if (lb_down && !ctrl_key) delta_rot = { (a.x > eps)*0.1f*d.x, (a.y > eps)*0.1f*d.y};
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_DOWN)  && (action == GLFW_PRESS)) delta_pos.y =  15.0f;
    if ((key == GLFW_KEY_UP)    && (action == GLFW_PRESS)) delta_pos.y = -15.0f;
    if ((key == GLFW_KEY_LEFT)  && (action == GLFW_PRESS)) delta_pos.x =  15.0f;
    if ((key == GLFW_KEY_RIGHT) && (action == GLFW_PRESS)) delta_pos.x = -15.0f;
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
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glsl_version = "#version 410";
    handle = glfwCreateWindow(1280, 720, "arbor-gui", NULL, NULL);
    if (handle == nullptr) log_fatal("Failed to obtain window");
    log_info("OpenGL version instantiated {}.{}",
             glfwGetWindowAttrib(handle, GLFW_CONTEXT_VERSION_MAJOR),
             glfwGetWindowAttrib(handle, GLFW_CONTEXT_VERSION_MINOR));
    glfwMakeContextCurrent(handle);
    glfwSwapInterval(1); // Enable vsync
    glfwSetScrollCallback(handle, scroll_callback);
    glfwSetCursorPosCallback(handle, mouse_callback);
    glfwSetKeyCallback(handle, key_callback);
    if (gl3wInit()) log_fatal("Failed to initialize OpenGL loader");

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

    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

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
    apply_style_dark();
    ImGui::PushFont(font);
}

void Window::end_frame() {
    ImGui::PopFont();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(handle);
    delta_pos = {0.0f, 0.0f};
    delta_rot = {0.0f, 0.0f};
    delta_zoom = 0.0f;
}

void Window::apply_style_dark() {
	ImGuiStyle* style = &ImGui::GetStyle();
	ImVec4* colors = style->Colors;

	colors[ImGuiCol_Text]                   = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.500f, 0.500f, 0.500f, 1.000f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.280f, 0.280f, 0.280f, 0.000f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_Border]                 = ImVec4(0.266f, 0.266f, 0.266f, 1.000f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.000f, 0.000f, 0.000f, 0.000f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.200f, 0.200f, 0.200f, 1.000f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.280f, 0.280f, 0.280f, 1.000f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.148f, 0.148f, 0.148f, 1.000f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.160f, 0.160f, 0.160f, 1.000f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.277f, 0.277f, 0.277f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.300f, 0.300f, 0.300f, 1.000f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_CheckMark]              = ImVec4(1.000f, 1.000f, 1.000f, 1.000f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Button]                 = ImVec4(1.000f, 1.000f, 1.000f, 0.000f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(1.000f, 1.000f, 1.000f, 0.391f);
	colors[ImGuiCol_Header]                 = ImVec4(0.313f, 0.313f, 0.313f, 1.000f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_Separator]              = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.391f, 0.391f, 0.391f, 1.000f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(1.000f, 1.000f, 1.000f, 0.250f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(1.000f, 1.000f, 1.000f, 0.670f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_Tab]                    = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabHovered]             = ImVec4(0.352f, 0.352f, 0.352f, 1.000f);
	colors[ImGuiCol_TabActive]              = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_TabUnfocused]           = ImVec4(0.098f, 0.098f, 0.098f, 1.000f);
	colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.195f, 0.195f, 0.195f, 1.000f);
	colors[ImGuiCol_DockingPreview]         = ImVec4(1.000f, 0.391f, 0.000f, 0.781f);
	colors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.180f, 0.180f, 0.180f, 1.000f);
	colors[ImGuiCol_PlotLines]              = ImVec4(0.469f, 0.469f, 0.469f, 1.000f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.586f, 0.586f, 0.586f, 1.000f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(1.000f, 1.000f, 1.000f, 0.156f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.000f, 0.391f, 0.000f, 1.000f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.000f, 0.000f, 0.000f, 0.586f);

	style->ChildRounding = 4.0f;
	style->FrameBorderSize = 1.0f;
	style->FrameRounding = 2.0f;
	style->GrabMinSize = 7.0f;
	style->PopupRounding = 2.0f;
	style->ScrollbarRounding = 12.0f;
	style->ScrollbarSize = 13.0f;
	style->TabBorderSize = 1.0f;
	style->TabRounding = 0.0f;
	style->WindowRounding = 4.0f;
}
