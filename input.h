#pragma once

#include <SDL2/SDL.h>
#include "osk.h"

#define DPAD_REPEAT_DELAY 300
#define DPAD_REPEAT_RATE  80

void input_set_pty_fd(int fd);

void input_dpad_tick(Uint32 now);

void input_reopen_controller(SDL_GameController **gc);

void input_handle_sdl_event(const SDL_Event *e, SDL_GameController **gc, int *running, int shell_dead, int *vis_rows, int term_h, int readonly);
