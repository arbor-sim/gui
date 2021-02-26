#include <gui_state.hpp>
#include <window.hpp>

#include <chrono>
#include <thread>

// target time for a frame, locked to 60Hz
using timer = std::chrono::high_resolution_clock;
constexpr auto frame_time = std::chrono::seconds(1)/60.0;

double to_us(auto dt) { return std::chrono::duration_cast<std::chrono::microseconds>(dt).count(); }

int main(int, char**) {
    log_init();

    log_info("Rendering locked to {} us/frame", to_us(frame_time));

    Window window{};
    gui_state state{};
    // Main loop
    auto loop = 0.0;
    auto time = 0.0;
    for (;window.running();) {
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
        ++loop;
        time += to_us(t2 - t0)*1e-6;
        // log_debug("Frame budget {} us; frame took {}; to sleep {} us; actually slept {} us; fps {}", to_us(frame_time), to_us(dt), to_us(frame_time - dt), to_us(t2 - t1), loop/time);
    }
}
