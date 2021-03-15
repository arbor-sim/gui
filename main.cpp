#include "gui_state.hpp"
#include "window.hpp"
#include "utils.hpp"

#include <chrono>
#include <thread>

int main(int, char**) {
    log_init();

    log_info("Rendering locked to {} us/frame", to_us(frame_time));

    Window window{};
    gui_state state{};

    for (;window.running() && !state.shutdown_requested;) {
        if (!window.visible()) {
            log_debug("Pausing for events.");
            glfwWaitEvents();
            continue;
        }
        auto t0 = timer::now();
        state.update();
        window.begin_frame();
        state.gui();
        window.end_frame();
        auto t1 = timer::now();
        auto dt = t1 - t0;
        if (dt < frame_time) std::this_thread::sleep_for(frame_time - dt);
        auto t2 = timer::now();
        FrameMark;
    }
}
