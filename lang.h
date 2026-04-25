#pragma once

typedef struct {

    const char *lang_code;
    const char *lang_name;
    const char *lang_author;
    const char *lang_version;

    const char *item_term_font_size;
    const char *item_menu_font_size;
    const char *item_font_hinting;
    const char *item_fg_colour;
    const char *item_bg_colour;
    const char *item_language;
    const char *item_save_config;
    const char *item_reset_config;
    const char *item_quit;

    const char *hint_normal;
    const char *hint_light;
    const char *hint_mono;
    const char *hint_none;

    const char *fg_white;
    const char *fg_amber;
    const char *fg_yellow;
    const char *fg_green;
    const char *fg_lime;
    const char *fg_cyan;
    const char *fg_light_blue;
    const char *fg_blue;
    const char *fg_pink;
    const char *fg_magenta;
    const char *fg_orange;
    const char *fg_light_gray;

    const char *bg_black;
    const char *bg_dark_gray;
    const char *bg_charcoal;
    const char *bg_dark_blue;
    const char *bg_navy;
    const char *bg_dark_green;
    const char *bg_forest;
    const char *bg_dark_purple;
    const char *bg_plum;
    const char *bg_dark_red;
    const char *bg_dark_teal;
    const char *bg_dark_brown;

    const char *menu_hint;

    const char *notify_config_saved;
    const char *notify_config_reset;

} LangDef;

extern const LangDef *g_lang;

extern const int g_lang_count;

extern const LangDef *const g_lang_table[];

void lang_init(void);

void lang_set(int idx);

int lang_current_index(void);