#include <iostream>
#include <vulkan/vulkan_raii.hpp>
#include "platform_render.h"
#include "definitions.h"
#include "game.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_events.h"

void update() {
	draw_frame();
}

int main() {
	std::cout << "hello world";

	if (!start_render()) {
		return 1;
	}

	u64 start_frame_time{};
	bool done = false;
    while (!done) {
		const u64 frame_time = 8000000; // In nanoseconds
		const u64 time_elapsed = SDL_GetTicksNS() - start_frame_time;
		if (time_elapsed < frame_time) {
			//std::cout << "sleeping for: " + std::to_string(frame_time - time_elapsed) << "\n";
			//SDL_DelayNS(frame_time - time_elapsed); // Do this if based on a setting or if VSync is busted
		}

		delta_time = (SDL_GetTicksNS() - start_frame_time) / 1000000000.0;
		start_frame_time = SDL_GetTicksNS();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
			switch(event.type) {
			case SDL_EVENT_QUIT: done = true; break;
			case SDL_EVENT_WINDOW_RESIZED: on_window_resized(); break;
			}
        }

        update();
    }

	end_render();
	return 0;
}
