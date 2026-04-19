#pragma once

#include <SDL2/SDL.h>

#define MUTERM_DEFAULT_WIDTH  640
#define MUTERM_DEFAULT_HEIGHT 480

#define MUTERM_DEFAULT_FONT_PATH "/opt/muos/share/font/muterm.ttf"
#define MUTERM_DEFAULT_FONT_SIZE 16

#define MUTERM_DEFAULT_SCROLLBACK 512

#define MUTERM_DEFAULT_SB_PATH "/tmp/muterm.cache"

#define MUOS_DEVICE_CONFIG "/opt/muos/device/config"
#define MUOS_GLOBAL_CONFIG "/opt/muos/config"

#define MUTERM_SYS_CONF "/opt/muos/share/conf/muterm.conf"
#define MUTERM_USR_CONF ".config/muterm/muterm.conf"

typedef struct {
    int width;
    int height;

    char font_path[512];
    char font_path_bold[512];
    char font_path_italic[512];
    char font_path_bold_italic[512];
    int font_size;

    int scrollback;
    char scrollback_path[512];

    int use_solid_bg;
    SDL_Color solid_bg;

    int use_solid_fg;
    SDL_Color solid_fg;

    char bg_image[512];
    int readonly;

    float zoom;
    int rotate;
    int underscan;

    char shell[256];

    char osk_layout_path[512];

    int ignore_muos;
} muTermConfig;

void config_load(muTermConfig *cfg, int ignore_muos);

void config_dump(const muTermConfig *cfg);