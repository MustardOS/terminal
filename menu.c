#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "menu.h"
#include "config.h"
#include "lang.h"

#define MENU_PAD     8
#define MENU_ROW_PAD 6

#define MENU_MIN_W 480
#define MENU_MAX_W 640

#define HINT_SCROLL_SPEED 1

#define NOTIFY_DURATION_MS 1600

static const SDL_Color COL_BACKDROP = {18, 18, 18, 230};
static const SDL_Color COL_BORDER = {90, 90, 110, 255};
static const SDL_Color COL_TITLE_FG = {255, 200, 0, 255};
static const SDL_Color COL_KEY_DARK = {40, 40, 40, 255};
static const SDL_Color COL_KEY_SEL = {255, 200, 0, 255};
static const SDL_Color COL_LBL_NORM = {255, 200, 0, 255};
static const SDL_Color COL_LBL_SEL = {0, 0, 0, 255};
static const SDL_Color COL_VAL_NORM = {180, 180, 180, 255};
static const SDL_Color COL_VAL_SEL = {0, 0, 0, 255};
static const SDL_Color COL_ARROW = {255, 200, 0, 180};
static const SDL_Color COL_HINT = {100, 100, 100, 255};
static const SDL_Color COL_NOTIFY = {255, 255, 255, 255};

typedef enum {
    ITEM_FONT_SIZE,
    ITEM_MENU_FONT_SIZE,
    ITEM_FONT_HINTING,
    ITEM_FG_COLOUR,
    ITEM_BG_COLOUR,
    ITEM_LANGUAGE,
    ITEM_SAVE_CONFIG,
    ITEM_RESET_CONFIG,
    ITEM_QUIT,
    ITEM_COUNT
} MenuItem;

static const char *item_label(MenuItem item) {
    switch (item) {
        case ITEM_FONT_SIZE:
            return g_lang->item_terminal_font_size;
        case ITEM_MENU_FONT_SIZE:
            return g_lang->item_menu_font_size;
        case ITEM_FONT_HINTING:
            return g_lang->item_font_hinting;
        case ITEM_FG_COLOUR:
            return g_lang->item_fg_colour;
        case ITEM_BG_COLOUR:
            return g_lang->item_bg_colour;
        case ITEM_LANGUAGE:
            return g_lang->item_language;
        case ITEM_SAVE_CONFIG:
            return g_lang->item_save_config;
        case ITEM_RESET_CONFIG:
            return g_lang->item_reset_config;
        case ITEM_QUIT:
            return g_lang->item_quit;
        default:
            return "";
    }
}

#define HINTING_COUNT 4

static const char *hinting_name(int h) {
    switch (h) {
        case 1:
            return g_lang->hint_light;
        case 2:
            return g_lang->hint_mono;
        case 3:
            return g_lang->hint_none;
        default:
            return g_lang->hint_normal;
    }
}

static const SDL_Color fg_colours[] = {
        {255, 255, 255, 255},   /* White      */
        {255, 200, 0,   255},   /* Amber      */
        {255, 240, 80,  255},   /* Yellow     */
        {50,  220, 80,  255},   /* Green      */
        {160, 230, 50,  255},   /* Lime       */
        {80,  220, 220, 255},   /* Cyan       */
        {140, 180, 255, 255},   /* Light Blue */
        {80,  120, 255, 255},   /* Blue       */
        {255, 140, 200, 255},   /* Pink       */
        {220, 80,  220, 255},   /* Magenta    */
        {255, 150, 40,  255},   /* Orange     */
        {200, 200, 200, 255},   /* Light Gray */
};

#define FG_PRESET_COUNT 12

static const char *fg_preset_name(int i) {
    switch (i) {
        case 0:
            return g_lang->fg_white;
        case 1:
            return g_lang->fg_amber;
        case 2:
            return g_lang->fg_yellow;
        case 3:
            return g_lang->fg_green;
        case 4:
            return g_lang->fg_lime;
        case 5:
            return g_lang->fg_cyan;
        case 6:
            return g_lang->fg_light_blue;
        case 7:
            return g_lang->fg_blue;
        case 8:
            return g_lang->fg_pink;
        case 9:
            return g_lang->fg_magenta;
        case 10:
            return g_lang->fg_orange;
        case 11:
            return g_lang->fg_light_gray;
        default:
            return "";
    }
}

static const SDL_Color bg_colours[] = {
        {0,  0,  0,  255},   /* Black       */
        {22, 22, 22, 255},   /* Dark Gray   */
        {35, 35, 35, 255},   /* Charcoal    */
        {8,  16, 40, 255},   /* Dark Blue   */
        {0,  10, 30, 255},   /* Navy        */
        {8,  28, 16, 255},   /* Dark Green  */
        {10, 25, 10, 255},   /* Forest      */
        {20, 8,  36, 255},   /* Dark Purple */
        {28, 8,  28, 255},   /* Plum        */
        {30, 8,  8,  255},   /* Dark Red    */
        {8,  28, 28, 255},   /* Dark Teal   */
        {28, 16, 4,  255},   /* Dark Brown  */
};

#define BG_PRESET_COUNT 12

static const char *bg_preset_name(int i) {
    switch (i) {
        case 0:
            return g_lang->bg_black;
        case 1:
            return g_lang->bg_dark_gray;
        case 2:
            return g_lang->bg_charcoal;
        case 3:
            return g_lang->bg_dark_blue;
        case 4:
            return g_lang->bg_navy;
        case 5:
            return g_lang->bg_dark_green;
        case 6:
            return g_lang->bg_forest;
        case 7:
            return g_lang->bg_dark_purple;
        case 8:
            return g_lang->bg_plum;
        case 9:
            return g_lang->bg_dark_red;
        case 10:
            return g_lang->bg_dark_teal;
        case 11:
            return g_lang->bg_dark_brown;
        default:
            return "";
    }
}

static int fg_preset_index(const SDL_Color *c) {
    for (int i = 0; i < FG_PRESET_COUNT; i++) {
        if (fg_colours[i].r == c->r &&
            fg_colours[i].g == c->g &&
            fg_colours[i].b == c->b)
            return i;
    }
    return 0;
}

static int bg_preset_index(const SDL_Color *c) {
    for (int i = 0; i < BG_PRESET_COUNT; i++) {
        if (bg_colours[i].r == c->r &&
            bg_colours[i].g == c->g &&
            bg_colours[i].b == c->b)
            return i;
    }
    return 0;
}

static void render_text(SDL_Renderer *ren, TTF_Font *font, const char *text, SDL_Color col, int x, int y) {
    SDL_Surface *s = TTF_RenderUTF8_Blended(font, text, col);
    if (!s) return;

    SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
    if (t) {
        SDL_Rect dst = {x, y, s->w, s->h};
        SDL_RenderCopy(ren, t, NULL, &dst);
        SDL_DestroyTexture(t);
    }

    SDL_FreeSurface(s);
}

static void item_value(const muTermConfig *cfg, MenuItem item, char *buf) {
    size_t sz = 64;

    switch (item) {
        case ITEM_FONT_SIZE:
            snprintf(buf, sz, "%d pt", cfg->font_size);
            break;
        case ITEM_MENU_FONT_SIZE:
            snprintf(buf, sz, "%d pt", cfg->menu_font_size);
            break;
        case ITEM_FONT_HINTING: {
            int h = cfg->font_hinting;
            if (h < 0 || h >= HINTING_COUNT) h = 0;
            snprintf(buf, sz, "%s", hinting_name(h));
            break;
        }
        case ITEM_FG_COLOUR:
            snprintf(buf, sz, "%s",
                     fg_preset_name(fg_preset_index(&cfg->solid_fg)));
            break;
        case ITEM_BG_COLOUR:
            snprintf(buf, sz, "%s",
                     bg_preset_name(bg_preset_index(&cfg->solid_bg)));
            break;
        case ITEM_LANGUAGE:
            snprintf(buf, sz, "%s", g_lang->lang_name);
            break;
        default:
            buf[0] = '\0';
            break;
    }
}

static int item_has_value(MenuItem item) {
    return item == ITEM_FONT_SIZE ||
           item == ITEM_MENU_FONT_SIZE ||
           item == ITEM_FONT_HINTING ||
           item == ITEM_FG_COLOUR ||
           item == ITEM_BG_COLOUR ||
           item == ITEM_LANGUAGE;
}

static void save_config(const muTermConfig *cfg) {
    FILE *f = fopen(MUTERM_SYS_CONF, "w");
    if (!f) {
        fprintf(stderr, "[MENU] cannot write %s\n", MUTERM_SYS_CONF);
        return;
    }

    static const char *const hint_names[] = {"normal", "light", "mono", "none"};

    int h = cfg->font_hinting;
    if (h < 0 || h > 3) h = 0;

    fprintf(f, "# muterm.conf -- saved by muTerm menu\n");
    fprintf(f, "font_size      = %d\n", cfg->font_size);
    fprintf(f, "menu_font_size = %d\n", cfg->menu_font_size);
    fprintf(f, "font_hinting   = %s\n", hint_names[h]);
    fprintf(f, "fg_colour      = %02X%02X%02X\n", cfg->solid_fg.r, cfg->solid_fg.g, cfg->solid_fg.b);
    fprintf(f, "bg_colour      = %02X%02X%02X\n", cfg->solid_bg.r, cfg->solid_bg.g, cfg->solid_bg.b);

    fclose(f);
    fprintf(stderr, "[MENU] config saved to %s\n", MUTERM_SYS_CONF);
}

static void reset_config(muTermConfig *cfg) {
    if (unlink(MUTERM_SYS_CONF) == 0) {
        fprintf(stderr, "[MENU] config reset (deleted %s)\n", MUTERM_SYS_CONF);
    } else {
        fprintf(stderr, "[MENU] config reset: %s not found\n", MUTERM_SYS_CONF);
    }

    cfg->font_size = MUTERM_DEFAULT_TERM_SIZE;
    cfg->menu_font_size = MUTERM_DEFAULT_MENU_SIZE;
    cfg->font_hinting = 0;

    cfg->solid_fg = (SDL_Color) {255, 255, 255, 255};
    cfg->use_solid_fg = 0;

    cfg->solid_bg = (SDL_Color) {0, 0, 0, 255};
    cfg->use_solid_bg = 0;
}

static MenuResult adjust_item(MenuItem item, int dir, muTermConfig *cfg, int *close_menu, int *do_quit) {
    MenuResult changed = MENU_RESULT_NONE;

    switch (item) {
        case ITEM_FONT_SIZE:
            cfg->font_size += dir;
            if (cfg->font_size < 6) cfg->font_size = 6;
            if (cfg->font_size > 72) cfg->font_size = 72;
            changed = MENU_RESULT_FONT;
            break;
        case ITEM_MENU_FONT_SIZE:
            cfg->menu_font_size += dir;
            if (cfg->menu_font_size < 8) cfg->menu_font_size = 8;
            if (cfg->menu_font_size > 32) cfg->menu_font_size = 32;
            changed = MENU_RESULT_FONT;
            break;
        case ITEM_FONT_HINTING:
            cfg->font_hinting = (cfg->font_hinting + HINTING_COUNT + dir) % HINTING_COUNT;
            changed = MENU_RESULT_FONT;
            break;
        case ITEM_FG_COLOUR: {
            int i = (fg_preset_index(&cfg->solid_fg) + FG_PRESET_COUNT + dir) % FG_PRESET_COUNT;
            cfg->solid_fg = fg_colours[i];
            cfg->use_solid_fg = 1;
            changed = MENU_RESULT_COLOUR;
            break;
        }
        case ITEM_BG_COLOUR: {
            int i = (bg_preset_index(&cfg->solid_bg) + BG_PRESET_COUNT + dir) % BG_PRESET_COUNT;
            cfg->solid_bg = bg_colours[i];
            cfg->use_solid_bg = 1;
            changed = MENU_RESULT_COLOUR;
            break;
        }
        case ITEM_LANGUAGE:
            lang_set((lang_current_index() + g_lang_count + dir) % g_lang_count);
            break;
        case ITEM_SAVE_CONFIG:
            save_config(cfg);
            break;
        case ITEM_RESET_CONFIG:
            reset_config(cfg);
            changed = MENU_RESULT_FONT;
            break;
        case ITEM_QUIT:
            *do_quit = 1;
            *close_menu = 1;
            break;
        default:
            break;
    }

    return changed;
}

static void menu_render(SDL_Renderer *ren, TTF_Font *font, int screen_w, int screen_h, const muTermConfig *cfg,
                        int sel, int hint_scroll_x, const char *notify_msg) {
    int row_h = 0, dummy = 0;
    TTF_SizeUTF8(font, "Ag", &dummy, &row_h);
    if (row_h <= 0) row_h = 16;

    int row_inner_h = row_h + MENU_ROW_PAD * 2;
    int row_stride = row_inner_h + 2;

    int panel_w = MENU_MAX_W;
    if (panel_w > screen_w - MENU_PAD * 2) panel_w = screen_w - MENU_PAD * 2;
    if (panel_w < MENU_MIN_W) panel_w = MENU_MIN_W;

    int panel_h = MENU_PAD + row_inner_h + MENU_PAD + ITEM_COUNT * row_stride + MENU_PAD + row_inner_h + MENU_PAD;

    int px = (screen_w - panel_w) / 2;
    int py = (screen_h - panel_h) / 2;

    int hint_inner_w = panel_w - MENU_PAD * 2;

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 160);
    SDL_Rect full = {0, 0, screen_w, screen_h};
    SDL_RenderFillRect(ren, &full);

    SDL_Rect panel = {px, py, panel_w, panel_h};
    SDL_SetRenderDrawColor(ren, COL_BACKDROP.r, COL_BACKDROP.g, COL_BACKDROP.b, COL_BACKDROP.a);
    SDL_RenderFillRect(ren, &panel);

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    SDL_SetRenderDrawColor(ren, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
    SDL_RenderDrawRect(ren, &panel);

    {
        int tw = 0, th = 0;
        TTF_SizeUTF8(font, " muTerm v" MUTERM_VERSION, &tw, &th);
        int ty = py + MENU_PAD + (row_inner_h - th) / 2;
        render_text(ren, font, " muTerm v" MUTERM_VERSION, COL_TITLE_FG, px + MENU_PAD, ty);
    }

    int items_y = py + MENU_PAD + row_inner_h + MENU_PAD;

    for (int i = 0; i < ITEM_COUNT; i++) {
        int iy = items_y + i * row_stride;
        int sel_ = (i == sel);

        SDL_Rect row_r = {px + 2, iy, panel_w - 4, row_inner_h};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren,
                               sel_ ? COL_KEY_SEL.r : COL_KEY_DARK.r,
                               sel_ ? COL_KEY_SEL.g : COL_KEY_DARK.g,
                               sel_ ? COL_KEY_SEL.b : COL_KEY_DARK.b,
                               255);
        SDL_RenderFillRect(ren, &row_r);

        SDL_SetRenderDrawColor(ren, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 200);
        SDL_RenderDrawRect(ren, &row_r);

        SDL_Color lc = sel_ ? COL_LBL_SEL : COL_LBL_NORM;

        int lx = px + MENU_PAD * 2;
        int lw = 0, lh = 0;

        TTF_SizeUTF8(font, item_label((MenuItem) i), &lw, &lh);

        int ly = iy + (row_inner_h - lh) / 2;
        render_text(ren, font, item_label((MenuItem) i), lc, lx, ly);

        if (item_has_value((MenuItem) i)) {
            char val[64] = {0};
            item_value(cfg, (MenuItem) i, val);

            int vw = 0, vh = 0;
            TTF_SizeUTF8(font, val, &vw, &vh);

            int aw = 0, ah = 0;
            TTF_SizeUTF8(font, "<", &aw, &ah);

            SDL_Color vc = sel_ ? COL_VAL_SEL : COL_VAL_NORM;

            int right_edge = px + panel_w - MENU_PAD * 2;
            int rx = right_edge - aw;
            int vx = rx - MENU_PAD - vw;
            int lax = vx - MENU_PAD - aw;

            int vy = iy + (row_inner_h - vh) / 2;
            int ay = iy + (row_inner_h - ah) / 2;

            if (sel_) render_text(ren, font, "<", COL_ARROW, lax, ay);
            render_text(ren, font, val, vc, vx, vy);
            if (sel_) render_text(ren, font, ">", COL_ARROW, rx, ay);
        }
    }

    if (notify_msg && notify_msg[0]) {
        int nw = 0, nh = 0;
        TTF_SizeUTF8(font, notify_msg, &nw, &nh);

        int toast_pad = MENU_PAD;

        int toast_w = nw + toast_pad * 2;
        int toast_h = nh + toast_pad * 2;

        int toast_x = px + (panel_w - toast_w) / 2;
        int toast_y = py + panel_h / 2 - toast_h / 2;

        SDL_Rect toast = {toast_x, toast_y, toast_w, toast_h};
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 30, 30, 30, 245);
        SDL_RenderFillRect(ren, &toast);

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(ren, COL_BORDER.r, COL_BORDER.g, COL_BORDER.b, 255);
        SDL_RenderDrawRect(ren, &toast);

        render_text(ren, font, notify_msg, COL_NOTIFY, toast_x + toast_pad, toast_y + toast_pad);
    }

    {
        const char *hint = g_lang->menu_hint;

        int hw = 0, hh = 0;
        TTF_SizeUTF8(font, hint, &hw, &hh);

        int hint_y = py + panel_h - MENU_PAD - row_inner_h + (row_inner_h - hh) / 2;
        int clip_x = px + MENU_PAD;

        SDL_Rect clip = {clip_x, py + panel_h - MENU_PAD - row_inner_h, hint_inner_w, row_inner_h};
        SDL_RenderSetClipRect(ren, &clip);

        if (hw <= hint_inner_w) {
            render_text(ren, font, hint, COL_HINT, clip_x, hint_y);
        } else {
            int gap = hint_inner_w / 3;
            int span = hw + gap;
            int x0 = clip_x - (hint_scroll_x % span);

            render_text(ren, font, hint, COL_HINT, x0, hint_y);
            render_text(ren, font, hint, COL_HINT, x0 + span, hint_y);
        }

        SDL_RenderSetClipRect(ren, NULL);
    }
}

MenuResult menu_open(SDL_Renderer *ren, TTF_Font **font_ptr, int screen_w, int screen_h, SDL_Texture *render_target,
                     const SDL_FRect *dest, double angle, muTermConfig *cfg, MenuPreviewFn preview_fn, void *preview_userdata) {
    TTF_Font *menu_font = *font_ptr;

    static int sel = 0;
    int close_menu = 0;
    int do_quit = 0;
    int hint_scroll = 0;
    char notify_msg[64] = {0};

    Uint32 notify_until = 0;
    MenuResult result = MENU_RESULT_NONE;

    SDL_GameController *gc = NULL;
    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            gc = SDL_GameControllerOpen(i);
            if (gc) break;
        }
    }

    int dpad_dir = 0;

    Uint32 dpad_press_t = 0;
    Uint32 dpad_last_rep = 0;

    Uint32 dpad_delay = (Uint32) cfg->dpad_repeat_delay;
    Uint32 dpad_rate = (Uint32) cfg->dpad_repeat_rate;

    SDL_Event e;

    while (!close_menu) {
        Uint32 now = SDL_GetTicks();

        if (notify_msg[0] && now >= notify_until) notify_msg[0] = '\0';

        if (dpad_dir != 0 && now - dpad_press_t >= dpad_delay && now - dpad_last_rep >= dpad_rate) {
            dpad_last_rep = now;
            if (dpad_dir < 0) {
                sel = (sel > 0) ? sel - 1 : ITEM_COUNT - 1;
            } else {
                sel = (sel + 1) % ITEM_COUNT;
            }
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopyExF(ren, render_target, NULL, dest, angle, NULL, SDL_FLIP_NONE);
        menu_render(ren, menu_font, screen_w, screen_h, cfg, sel, hint_scroll, notify_msg[0] ? notify_msg : NULL);
        SDL_RenderPresent(ren);

        hint_scroll += HINT_SCROLL_SPEED;

        if (!SDL_PollEvent(&e)) {
            SDL_Delay(16);
            continue;
        }

        switch (e.type) {
            case SDL_QUIT:
                do_quit = 1;
                close_menu = 1;
                break;
            case SDL_CONTROLLERBUTTONDOWN:
                switch (e.cbutton.button) {
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        sel = (sel > 0) ? sel - 1 : ITEM_COUNT - 1;
                        dpad_dir = -1;
                        dpad_press_t = now;
                        dpad_last_rep = now;
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        sel = (sel + 1) % ITEM_COUNT;
                        dpad_dir = +1;
                        dpad_press_t = now;
                        dpad_last_rep = now;
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT: {
                        MenuResult r = adjust_item((MenuItem) sel, -1, cfg, &close_menu, &do_quit);
                        if (r != MENU_RESULT_NONE) {
                            result = r;
                            if (preview_fn) {
                                preview_fn(r, preview_userdata);
                                menu_font = *font_ptr;
                            }
                        }

                        if ((MenuItem) sel == ITEM_SAVE_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_saved);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        } else if ((MenuItem) sel == ITEM_RESET_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_reset);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        }

                        break;
                    }
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                    case SDL_CONTROLLER_BUTTON_A:
                    case SDL_CONTROLLER_BUTTON_LEFTSTICK:
                    case SDL_CONTROLLER_BUTTON_START: {
                        MenuResult r = adjust_item((MenuItem) sel, +1, cfg, &close_menu, &do_quit);
                        if (r != MENU_RESULT_NONE) {
                            result = r;
                            if (preview_fn) {
                                preview_fn(r, preview_userdata);
                                menu_font = *font_ptr;
                            }
                        }

                        if ((MenuItem) sel == ITEM_SAVE_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_saved);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        } else if ((MenuItem) sel == ITEM_RESET_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_reset);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        }

                        break;
                    }
                    case SDL_CONTROLLER_BUTTON_B:
                    case SDL_CONTROLLER_BUTTON_GUIDE:
                    case SDL_CONTROLLER_BUTTON_BACK:
                        close_menu = 1;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_CONTROLLERBUTTONUP:
                if (e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP || e.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) dpad_dir = 0;
                break;
            case SDL_JOYBUTTONDOWN:
                if (gc) break;
                switch (e.jbutton.button) {
                    case 1:
                        sel = (sel > 0) ? sel - 1 : ITEM_COUNT - 1;
                        break;
                    case 2:
                        sel = (sel + 1) % ITEM_COUNT;
                        break;
                    case 3:
                    case 10: {
                        MenuResult r = adjust_item((MenuItem) sel, +1, cfg, &close_menu, &do_quit);
                        if (r != MENU_RESULT_NONE) {
                            result = r;
                            if (preview_fn) {
                                preview_fn(r, preview_userdata);
                                menu_font = *font_ptr;
                            }
                        }

                        if ((MenuItem) sel == ITEM_SAVE_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_saved);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        } else if ((MenuItem) sel == ITEM_RESET_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_reset);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        }

                        break;
                    }
                    case 4:
                    case 9:
                    case 11:
                        close_menu = 1;
                        break;
                    default:
                        break;
                }
                break;
            case SDL_KEYDOWN:
                switch (e.key.keysym.sym) {
                    case SDLK_UP:
                        sel = (sel > 0) ? sel - 1 : ITEM_COUNT - 1;
                        break;
                    case SDLK_DOWN:
                        sel = (sel + 1) % ITEM_COUNT;
                        break;
                    case SDLK_LEFT: {
                        MenuResult r = adjust_item((MenuItem) sel, -1, cfg, &close_menu, &do_quit);
                        if (r != MENU_RESULT_NONE) {
                            result = r;
                            if (preview_fn) {
                                preview_fn(r, preview_userdata);
                                menu_font = *font_ptr;
                            }
                        }

                        if ((MenuItem) sel == ITEM_SAVE_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_saved);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        } else if ((MenuItem) sel == ITEM_RESET_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_reset);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        }

                        break;
                    }
                    case SDLK_RIGHT:
                    case SDLK_RETURN:
                    case SDLK_KP_ENTER: {
                        MenuResult r = adjust_item((MenuItem) sel, +1, cfg, &close_menu, &do_quit);
                        if (r != MENU_RESULT_NONE) {
                            result = r;
                            if (preview_fn) {
                                preview_fn(r, preview_userdata);
                                menu_font = *font_ptr;
                            }
                        }

                        if ((MenuItem) sel == ITEM_SAVE_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_saved);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        } else if ((MenuItem) sel == ITEM_RESET_CONFIG) {
                            snprintf(notify_msg, sizeof(notify_msg), "%s", g_lang->notify_config_reset);
                            notify_until = SDL_GetTicks() + NOTIFY_DURATION_MS;
                        }

                        break;
                    }
                    case SDLK_ESCAPE:
                    case SDLK_BACKSPACE:
                        close_menu = 1;
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }

    if (gc) SDL_GameControllerClose(gc);
    if (do_quit) return MENU_RESULT_QUIT;

    return result;
}
