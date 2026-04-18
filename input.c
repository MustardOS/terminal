#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "input.h"
#include "osk.h"
#include "vt.h"

static int g_pty_fd = -1;

typedef enum {
    DPAD_NONE = 0,
    DPAD_UP = 1,
    DPAD_DOWN = 2,
    DPAD_LEFT = 3,
    DPAD_RIGHT = 4,
} DpadDir;

static DpadDir g_dpad_held = DPAD_NONE;
static Uint32 g_dpad_press_t = 0;
static Uint32 g_dpad_last_rep = 0;

static void pty_write(const char *data, int len) {
    if (g_pty_fd >= 0 && len > 0) {
        ssize_t r = write(g_pty_fd, data, (size_t) len);
        (void) r;
    }
}

void input_set_pty_fd(int fd) {
    g_pty_fd = fd;
}

static void dpad_osk_move(DpadDir dir) {
    switch (dir) {
        case DPAD_UP:
            osk_move(-1, 0);
            break;
        case DPAD_DOWN:
            osk_move(1, 0);
            break;
        case DPAD_LEFT:
            osk_move(0, -1);
            break;
        case DPAD_RIGHT:
            osk_move(0, 1);
            break;
        default:
            break;
    }
}

void input_dpad_tick(Uint32 now) {
    if (g_dpad_held == DPAD_NONE || !osk_is_visible()) return;

    if (now - g_dpad_press_t >= DPAD_REPEAT_DELAY &&
        now - g_dpad_last_rep >= DPAD_REPEAT_RATE) {
        g_dpad_last_rep = now;
        dpad_osk_move(g_dpad_held);
    }
}

static void handle_keyboard(SDL_KeyboardEvent *key, int readonly) {
    if (readonly) return;

    SDL_Keycode sym = key->keysym.sym;
    SDL_Keymod mods = SDL_GetModState();

    int ctrl = (mods & (KMOD_LCTRL | KMOD_RCTRL)) != 0;
    int shift = (mods & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
    int alt = (mods & (KMOD_LALT | KMOD_RALT)) != 0;

    if (sym == SDLK_UP || sym == SDLK_DOWN || sym == SDLK_RIGHT || sym == SDLK_LEFT) {
        char seq[4] = {'\x1B', vt_cursor_keys_app() ? 'O' : '[', 'A', '\0'};

        switch (sym) {
            case SDLK_UP:
                seq[2] = 'A';
                break;
            case SDLK_DOWN:
                seq[2] = 'B';
                break;
            case SDLK_RIGHT:
                seq[2] = 'C';
                break;
            default:
                seq[2] = 'D';
                break;
        }

        pty_write(seq, 3);
        return;
    }

    static const struct {
        SDL_Keycode k;
        const char *s;
        int n;
    } specials[] = {
            {SDLK_HOME,     "\x1B[H",  3},
            {SDLK_END,      "\x1B[F",  3},
            {SDLK_PAGEUP,   "\x1B[5~", 4},
            {SDLK_PAGEDOWN, "\x1B[6~", 4},
            {SDLK_INSERT,   "\x1B[2~", 4},
            {SDLK_DELETE,   "\x1B[3~", 4},
    };

    for (int i = 0; i < (int) (sizeof(specials) / sizeof(specials[0])); i++) {
        if (sym == specials[i].k) {
            pty_write(specials[i].s, specials[i].n);
            return;
        }
    }

    static const char *fn_keys[] = {
            "\x1BOP", "\x1BOQ", "\x1BOR", "\x1BOS",
            "\x1B[15~", "\x1B[17~", "\x1B[18~", "\x1B[19~",
            "\x1B[20~", "\x1B[21~", "\x1B[23~", "\x1B[24~",
    };

    if (sym >= SDLK_F1 && sym <= SDLK_F12) {
        int fi = sym - SDLK_F1;
        pty_write(fn_keys[fi], (int) strlen(fn_keys[fi]));
        return;
    }

    if (ctrl) {
        if (sym >= SDLK_a && sym <= SDLK_z) {
            char cc = (char) (sym - SDLK_a + 1);
            pty_write(&cc, 1);
            return;
        }

        switch (sym) {
            case SDLK_SPACE: {
                char cc = 0x00;
                pty_write(&cc, 1);
                return;
            }
            case SDLK_LEFTBRACKET: {
                char cc = 0x1B;
                pty_write(&cc, 1);
                return;
            }
            case SDLK_BACKSLASH: {
                char cc = 0x1C;
                pty_write(&cc, 1);
                return;
            }
            case SDLK_RIGHTBRACKET: {
                char cc = 0x1D;
                pty_write(&cc, 1);
                return;
            }
            case SDLK_6: {
                char cc = 0x1E;
                pty_write(&cc, 1);
                return;
            }
            case SDLK_SLASH:
            case SDLK_7: {
                char cc = 0x1F;
                pty_write(&cc, 1);
                return;
            }
            case SDLK_2: {
                char cc = 0x00;
                pty_write(&cc, 1);
                return;
            }
            default:
                break;
        }
    }

    if (alt) {
        char esc = '\x1B';
        pty_write(&esc, 1);
    }

    switch (sym) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: {
            char c = '\r';
            pty_write(&c, 1);
            return;
        }
        case SDLK_BACKSPACE: {
            char c = '\x7F';
            pty_write(&c, 1);
            return;
        }
        case SDLK_TAB:
            if (shift) { pty_write("\x1B[Z", 3); }
            else {
                char c = '\t';
                pty_write(&c, 1);
            }
            return;
        case SDLK_ESCAPE: {
            char c = '\x1B';
            pty_write(&c, 1);
            return;
        }
        default:
            break;
    }
}

static input_action_t map_controller_button(Uint8 button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_BACK:
            return INPUT_ACT_OSK_TOGGLE;
        case SDL_CONTROLLER_BUTTON_GUIDE:
            return INPUT_ACT_QUIT;
        case SDL_CONTROLLER_BUTTON_A:
            return INPUT_ACT_PRESS;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK:
            return INPUT_ACT_PRESS;
        case SDL_CONTROLLER_BUTTON_B:
            return INPUT_ACT_BACKSPACE;
        case SDL_CONTROLLER_BUTTON_Y:
            return INPUT_ACT_SPACE;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            return INPUT_ACT_LAYER_PREV;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            return INPUT_ACT_LAYER_NEXT;
        case SDL_CONTROLLER_BUTTON_START:
            return INPUT_ACT_ENTER;
        default:
            return INPUT_ACT_NONE;
    }
}

static input_action_t map_joystick_button(int raw) {
    switch (raw) {
        case 9:
            return INPUT_ACT_OSK_TOGGLE;
        case 11:
            return INPUT_ACT_QUIT;
        case 3:
            return INPUT_ACT_PRESS;
        case 4:
            return INPUT_ACT_BACKSPACE;
        case 5:
            return INPUT_ACT_SPACE;
        case 7:
            return INPUT_ACT_LAYER_PREV;
        case 8:
            return INPUT_ACT_LAYER_NEXT;
        case 10:
            return INPUT_ACT_ENTER;
        default:
            return INPUT_ACT_NONE;
    }
}

static int event_from_active_controller(const SDL_Event *e, SDL_GameController *gc) {
    if (!gc) return 0;

    SDL_Joystick *joy = SDL_GameControllerGetJoystick(gc);
    if (!joy) return 0;

    SDL_JoystickID which = SDL_JoystickInstanceID(joy);

    switch (e->type) {
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP:
            return e->cbutton.which == which;
        case SDL_JOYAXISMOTION:
            return e->jaxis.which == which;
        case SDL_JOYHATMOTION:
            return e->jhat.which == which;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            return e->jbutton.which == which;
        default:
            return 0;
    }
}

void input_reopen_controller(SDL_GameController **gc) {
    if (*gc) {
        SDL_GameControllerClose(*gc);
        *gc = NULL;
    }

    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            *gc = SDL_GameControllerOpen(i);
            if (*gc) {
                fprintf(stderr, "[SDL] GameController %d: %s\n", i, SDL_GameControllerName(*gc));
                break;
            }
        }
    }
}

static void handle_dpad_button_down(DpadDir dir, int osk_visible) {
    static const char arrow_codes[5] = {0, 'A', 'B', 'D', 'C'};

    if (osk_visible) {
        Uint32 now = SDL_GetTicks();
        g_dpad_held = dir;
        g_dpad_press_t = now;
        g_dpad_last_rep = now;
        dpad_osk_move(dir);
    } else {
        char seq[3] = {'\x1B', vt_cursor_keys_app() ? 'O' : '[', arrow_codes[dir]};
        pty_write(seq, 3);
    }
}

void input_handle_sdl_event(const SDL_Event *e, SDL_GameController **gc, int *running, int shell_dead, int *vis_rows, int term_h, int readonly) {
    switch (e->type) {
        case SDL_QUIT:
            *running = 0;
            return;
        case SDL_TEXTINPUT:
            if (!readonly && !osk_is_visible()) {
                pty_write(e->text.text, (int) strlen(e->text.text));
                vt_scroll_set(0);
            }
            return;
        case SDL_KEYDOWN:
            if (e->key.keysym.sym == SDLK_ESCAPE &&
                !(SDL_GetModState() & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT)) &&
                shell_dead) {
                *running = 0;
                return;
            }

            if (e->key.keysym.sym == SDLK_PAGEUP) {
                vt_scroll_adjust(*vis_rows / 2, *vis_rows);
                return;
            }

            if (e->key.keysym.sym == SDLK_PAGEDOWN) {
                vt_scroll_adjust(-(*vis_rows / 2), *vis_rows);
                return;
            }

            handle_keyboard((SDL_KeyboardEvent *) &e->key, readonly);
            return;
        case SDL_CONTROLLERBUTTONDOWN: {
            Uint8 btn = e->cbutton.button;

            if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP) {
                handle_dpad_button_down(DPAD_UP, osk_is_visible());
                return;
            }
            if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                handle_dpad_button_down(DPAD_DOWN, osk_is_visible());
                return;
            }
            if (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
                handle_dpad_button_down(DPAD_LEFT, osk_is_visible());
                return;
            }
            if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
                handle_dpad_button_down(DPAD_RIGHT, osk_is_visible());
                return;
            }

            osk_apply_action(map_controller_button(btn), running, vis_rows, term_h, readonly, g_pty_fd);
            return;
        }
        case SDL_CONTROLLERBUTTONUP: {
            Uint8 btn = e->cbutton.button;

            if (btn == SDL_CONTROLLER_BUTTON_DPAD_UP ||
                btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN ||
                btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT ||
                btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
                g_dpad_held = DPAD_NONE;
                return;
            }

            if (btn == SDL_CONTROLLER_BUTTON_A ||
                btn == SDL_CONTROLLER_BUTTON_B ||
                btn == SDL_CONTROLLER_BUTTON_Y ||
                btn == SDL_CONTROLLER_BUTTON_LEFTSTICK) {
                osk_hold_end();
            }
            return;
        }
        case SDL_JOYBUTTONDOWN: {
            int raw = (int) e->jbutton.button;
            input_action_t a = map_joystick_button(raw);

            if (raw == 1 || raw == 2) {
                osk_apply_action(a, running, vis_rows, term_h, readonly, g_pty_fd);
                return;
            }

            if (*gc) return;

            if (a != INPUT_ACT_NONE) osk_apply_action(a, running, vis_rows, term_h, readonly, g_pty_fd);
            return;
        }
        case SDL_JOYBUTTONUP: {
            int raw = (int) e->jbutton.button;
            if (raw == 3 || raw == 4 || raw == 5) osk_hold_end();
            return;
        }
        case SDL_JOYAXISMOTION:
            if (*gc && event_from_active_controller(e, *gc)) return;
            osk_handle_axis(e->jaxis.axis, e->jaxis.value);
            return;
        case SDL_CONTROLLERAXISMOTION: {
            static int lt_active = 0;
            static int rt_active = 0;

            const int dead = 16000;

            if (e->caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT) {
                if (e->caxis.value > dead) {
                    if (!lt_active) {
                        vt_scroll_adjust(*vis_rows / 2, *vis_rows);
                        lt_active = 1;
                    }
                } else {
                    lt_active = 0;
                }
                return;
            }

            if (e->caxis.axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT) {
                if (e->caxis.value > dead) {
                    if (!rt_active) {
                        vt_scroll_adjust(-(*vis_rows / 2), *vis_rows);
                        rt_active = 1;
                    }
                } else {
                    rt_active = 0;
                }
                return;
            }

            if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ||
                e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                osk_handle_axis(e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 1, e->caxis.value);
            }
            return;
        }
        case SDL_JOYHATMOTION:
            if (*gc) return;
            if (osk_is_visible()) {
                osk_handle_hat(e->jhat.value);
            } else {
                if (e->jhat.value & SDL_HAT_UP) vt_scroll_adjust(1, *vis_rows);
                else if (e->jhat.value & SDL_HAT_DOWN) vt_scroll_adjust(-1, *vis_rows);
                else if (e->jhat.value & SDL_HAT_LEFT) vt_scroll_adjust(*vis_rows / 2, *vis_rows);
                else if (e->jhat.value & SDL_HAT_RIGHT) vt_scroll_adjust(-(*vis_rows / 2), *vis_rows);
            }
            return;
        case SDL_CONTROLLERDEVICEADDED:
            if (!*gc) input_reopen_controller(gc);
            return;
        case SDL_CONTROLLERDEVICEREMOVED:
            input_reopen_controller(gc);
            return;
        default:
            break;
    }
}
