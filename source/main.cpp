#include <vulkan/vulkan_raii.hpp>
#include "platform_render.h"
#include "definitions.h"
#include "game.h"
#include "SDL3/SDL_timer.h"
#include "SDL3/SDL_events.h"
#include "input.h"

void update_camera_direction() {
	static constexpr float MOUSE_MOVE_SPEED = 0.5f;
	const Vector3 right = Vector3::cross(camera_direction, Vector3::up());

	float mouse_x; float mouse_y;
	SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

	camera_direction += right * MOUSE_MOVE_SPEED * mouse_x * delta_time;
	if (mouse_y > 0.f && Vector3::angle(camera_direction, Vector3::down()) > M_PI / 8.f ||
		mouse_y < 0.f && Vector3::angle(camera_direction, Vector3::up()) > M_PI / 8.f) {
		camera_direction -= Vector3::up() * MOUSE_MOVE_SPEED * mouse_y * delta_time;
	}

	camera_direction = camera_direction.normalized();
}

void update_movement() {
	static constexpr float speed = 3.f;
	const Vector3 right = Vector3::cross(camera_direction, Vector3::up());
	Vector3 movement_direction{};

	if (input_down_this_frame(InputType::FORWARD)) {
		movement_direction -= camera_direction;
	}
	if (input_down_this_frame(InputType::BACKWARD)) {
		movement_direction += camera_direction;
	}
	if (input_down_this_frame(InputType::RIGHT)) {
		movement_direction -= right;
	}
	if (input_down_this_frame(InputType::LEFT)) {
		movement_direction += right;
	}

	if (movement_direction == Vector3::zero()) {
		return;
	}

	camera_position += movement_direction.normalized() * speed * delta_time;
}

void update() {
	update_input();
	update_camera_direction();
	update_movement();
	draw_frame();
}

int main() {
	if (!start_render()) {
		return 1;
	}

	SDL_SetWindowRelativeMouseMode(get_window(), true);

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
