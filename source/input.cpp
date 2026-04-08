#include "input.h"
#include <SDL3/SDL.h>

static Input inputs[NUMBER_OF_INPUT_TYPES] {};

static void update_input_state(InputType input_type, bool is_down) {
	int index = static_cast<int>(input_type);
	if (inputs[index].is_down != is_down) {
		inputs[index].is_down = is_down;
		inputs[index].is_handled = false;
	}
}

void update_input() {
	const bool* key_states = SDL_GetKeyboardState(nullptr);

	update_input_state(InputType::FORWARD, key_states[SDL_SCANCODE_W]);
	update_input_state(InputType::BACKWARD, key_states[SDL_SCANCODE_S]);
	update_input_state(InputType::LEFT, key_states[SDL_SCANCODE_A]);
	update_input_state(InputType::RIGHT, key_states[SDL_SCANCODE_D]);

	float mouse_x; float mouse_y;
	SDL_MouseButtonFlags mouse_state = SDL_GetMouseState(&mouse_x, &mouse_y);
	update_input_state(InputType::MOUSE_CLICK, (mouse_state & SDL_BUTTON_LEFT) > 0);
}

bool input_held(InputType input_type) {
	int index = static_cast<int>(input_type);
	return inputs[index].is_down;
}

bool input_down_this_frame(InputType input_type) {
	int index = static_cast<int>(input_type);
	return inputs[index].is_down && !inputs[index].is_handled;
}

bool input_released_this_frame(InputType input_type) {
	int index = static_cast<int>(input_type);
	return !inputs[index].is_down && !inputs[index].is_handled;
}

void handle_input(InputType input_type) {
	inputs[static_cast<int>(input_type)].is_handled = true;
}

void input_end_frame() {
	for (Input& input : inputs) {
		input.is_handled = true;
	}
}
