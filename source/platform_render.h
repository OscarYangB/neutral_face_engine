#pragma once

struct SDL_Window;

bool start_render();
void end_render();
void draw_frame();
void on_window_resized();

SDL_Window* get_window();
