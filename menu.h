#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "config.h"

typedef enum {
    MENU_RESULT_NONE   = 0,
    MENU_RESULT_QUIT   = 1,
    MENU_RESULT_FONT   = 2,
    MENU_RESULT_COLOUR = 3,
} MenuResult;

typedef void (*MenuPreviewFn)(MenuResult what, void *userdata);

MenuResult menu_open(SDL_Renderer *ren, TTF_Font **font_ptr, int screen_w, int screen_h, SDL_Texture *render_target,
                     const SDL_FRect *dest, double angle, muTermConfig *cfg, MenuPreviewFn preview_fn, void *preview_userdata);
