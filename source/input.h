#pragma once

#include "definitions.h"

struct SDL_KeyboardEvent;

enum class InputType {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN,
	MOUSE_CLICK,
	_count
};

constexpr u8 NUMBER_OF_INPUT_TYPES = static_cast<int>(InputType::_count);

struct Input {
	bool is_down = false;
	bool is_handled = false;
};

void update_input();
void handle_input_event(SDL_KeyboardEvent event);

bool input_held(InputType input_type);
bool input_down_this_frame(InputType input_type);
bool input_released_this_frame(InputType input_type);
void handle_input(InputType input_type);
void input_end_frame();
