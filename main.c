#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <locale.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "config.h"
#include "vt.h"
#include "render.h"
#include "osk.h"

static int CELL_WIDTH = 0;
static int CELL_HEIGHT = 0;

static int pty_fd_global = -1;

static void pty_write(const char *data, int len) {
    if (pty_fd_global >= 0 && len > 0) {
        ssize_t r = write(pty_fd_global, data, (size_t) len);
        (void) r;
    }
}

static volatile sig_atomic_t child_exited = 0;

static void sigchld_handler(int s) {
    (void) s;
    child_exited = 1;
}

static pid_t spawn_pty_child(int *master_fd_out, int argc, char **argv, const char *shell_override) {
    int master_fd = -1, slave_fd = -1;

    struct winsize ws = {
            .ws_row = (unsigned short) vt_rows(),
            .ws_col = (unsigned short) vt_cols(),
    };

    if (openpty(&master_fd, &slave_fd, NULL, NULL, &ws) < 0) {
        perror("openpty");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(master_fd);
        close(slave_fd);
        return -1;
    }

    if (pid == 0) {
        setsid();
        if (ioctl(slave_fd, TIOCSCTTY, 0) < 0) perror("TIOCSCTTY");

        if (dup2(slave_fd, STDIN_FILENO) < 0 ||
            dup2(slave_fd, STDOUT_FILENO) < 0 ||
            dup2(slave_fd, STDERR_FILENO) < 0) {
            perror("dup2");
            _exit(1);
        }

        close(master_fd);
        close(slave_fd);

        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

        if (!getenv("HOME")) setenv("HOME", "/root", 1);

        if (argc > 1 && argv[1] && argv[1][0]) {
            execvp(argv[1], &argv[1]);
            perror("execvp");
            _exit(127);
        }

        const char *shell = (shell_override && *shell_override) ? shell_override : getenv("SHELL");
        if (!shell || !*shell) shell = "/bin/sh";

        execlp(shell, shell, "-l", (char *) NULL);
        perror("execlp");
        _exit(127);
    }

    close(slave_fd);
    *master_fd_out = master_fd;

    return pid;
}

static void handle_keyboard(SDL_KeyboardEvent *key, int readonly) {
    if (readonly) return;

    SDL_Keycode sym = key->keysym.sym;
    SDL_Keymod mods = SDL_GetModState();

    int ctrl = (mods & (KMOD_LCTRL | KMOD_RCTRL)) != 0;
    int shift = (mods & (KMOD_LSHIFT | KMOD_RSHIFT)) != 0;
    int alt = (mods & (KMOD_LALT | KMOD_RALT)) != 0;

    if (sym == SDLK_UP || sym == SDLK_DOWN || sym == SDLK_RIGHT || sym == SDLK_LEFT) {
        char seq[4] = {'\x1B', '[', 'A', '\0'};

        if (sym == SDLK_UP) {
            seq[2] = 'A';
        } else if (sym == SDLK_DOWN) {
            seq[2] = 'B';
        } else if (sym == SDLK_RIGHT) {
            seq[2] = 'C';
        } else {
            seq[2] = 'D';
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
            "\x1B[20~", "\x1B[21~", "\x1B[23~", "\x1B[24~"
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

        if (sym == SDLK_SPACE) {
            char cc = 0;
            pty_write(&cc, 1);
            return;
        }

        if (sym == SDLK_BACKSLASH) {
            char cc = 0x1C;
            pty_write(&cc, 1);
            return;
        }

        if (sym == SDLK_RIGHTBRACKET) {
            char cc = 0x1D;
            pty_write(&cc, 1);
            return;
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
            if (shift) pty_write("\x1B[Z", 3);
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

/* ── controller mapping ───────────────────────────────────────────────── */

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
        case 2:
            return INPUT_ACT_PAGE_UP;
        case 1:
            return INPUT_ACT_PAGE_DOWN;
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

static void reopen_controller(SDL_GameController **gc) {
    if (*gc) {
        SDL_GameControllerClose(*gc);
        *gc = NULL;
    }

    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            *gc = SDL_GameControllerOpen(i);
            if (*gc) {
                fprintf(stderr, "[OSK] Opened GameController %d: %s\n", i, SDL_GameControllerName(*gc));
                break;
            }
        }
    }
}

static void handle_sdl_event(const SDL_Event *e, SDL_GameController **gc, int *running, int shell_dead, int *vis_rows, int term_h, int readonly) {
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
                !(SDL_GetModState() & (KMOD_CTRL | KMOD_ALT | KMOD_SHIFT))) {
                if (shell_dead) {
                    *running = 0;
                    return;
                }
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
                osk_move(-1, 0);
                break;
            }

            if (btn == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
                osk_move(1, 0);
                break;
            }

            if (btn == SDL_CONTROLLER_BUTTON_DPAD_LEFT) {
                osk_move(0, -1);
                break;
            }

            if (btn == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) {
                osk_move(0, 1);
                break;
            }

            input_action_t a = map_controller_button(btn);
            osk_apply_action(a, running, vis_rows, term_h, readonly, pty_fd_global);
            break;
        }
        case SDL_JOYBUTTONDOWN: {
            int raw = (int) e->jbutton.button;
            if (raw == 1 || raw == 2) {
                input_action_t a = map_joystick_button(raw);
                osk_apply_action(a, running, vis_rows, term_h, readonly, pty_fd_global);
                return;
            }

            if (*gc) return;

            input_action_t a = map_joystick_button(raw);
            if (a != INPUT_ACT_NONE) osk_apply_action(a, running, vis_rows, term_h, readonly, pty_fd_global);

            return;
        }
        case SDL_CONTROLLERBUTTONUP: {
            Uint8 btn = e->cbutton.button;
            if (btn == SDL_CONTROLLER_BUTTON_A ||
                btn == SDL_CONTROLLER_BUTTON_B ||
                btn == SDL_CONTROLLER_BUTTON_Y ||
                btn == SDL_CONTROLLER_BUTTON_LEFTSTICK)
                osk_hold_end();
            return;
        }
        case SDL_JOYBUTTONUP: {
            int raw = (int) e->jbutton.button;
            /* FIX 2: end hold for A (3), B (4), and Y (5) — previously only button 3 was handled */
            if (raw == 3 || raw == 4 || raw == 5)
                osk_hold_end();
            return;
        }
        case SDL_JOYAXISMOTION:
            if (*gc && event_from_active_controller(e, *gc)) return;
            osk_handle_axis(e->jaxis.axis, e->jaxis.value);
            return;
        case SDL_CONTROLLERAXISMOTION:
            if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX || e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) {
                osk_handle_axis(e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX ? 0 : 1, e->caxis.value);
            }
            return;
        case SDL_JOYHATMOTION:
            if (*gc) return;

            osk_handle_hat(e->jhat.value);
            return;
        case SDL_CONTROLLERDEVICEADDED:
            if (!*gc) reopen_controller(gc);
            return;
        case SDL_CONTROLLERDEVICEREMOVED:
            reopen_controller(gc);
            return;
        default:
            break;
    }
}

static void print_help(const char *name) {
    printf("muterm – portable virtual terminal\n\n");
    printf("Usage:\n\t%s [options] [-- command [args...]]\n\n", name);
    printf("Options:\n");
    printf("\t-w, --width  <px>        Window width   (default: from config / %d)\n", MUTERM_DEFAULT_WIDTH);
    printf("\t-h, --height <px>        Window height  (default: from config / %d)\n", MUTERM_DEFAULT_HEIGHT);
    printf("\t-s, --size   <pt>        Font size      (default: from config / %d)\n", MUTERM_DEFAULT_FONT_SIZE);
    printf("\t-f, --font   <path.ttf>  Font file      (default: from config / built-in)\n");
    printf("\t-i, --image  <file>      Background PNG\n");
    printf("\t-sb,--scrollback <n>     Scrollback lines (default: %d)\n", MUTERM_DEFAULT_SCROLLBACK);
    printf("\t-bg,--bgcolour RRGGBB    Solid background colour\n");
    printf("\t-fg,--fgcolour RRGGBB    Solid foreground colour (overrides ANSI)\n");
    printf("\t-ro,--readonly           View-only: display PTY output, no input\n");
    printf("\t--zoom <factor>          Display zoom  (e.g. 1.5)\n");
    printf("\t--rotate <0-3>           Rotate display (0=none 1=90 2=180 3=270)\n");
    printf("\t--underscan              Apply HDMI underscan (16 px)\n");
    printf("\t--no-underscan           Disable underscan\n\n");
    printf("Config files (lower entries override higher):\n");
    printf("\t%s\n\t%s\n\t%s\n\t$HOME/%s\n\n",
           MUOS_DEVICE_CONFIG, MUOS_GLOBAL_CONFIG,
           MUTERM_SYS_CONF, MUTERM_USR_CONF);
    printf("Controls:\n");
    printf("\tSelect       Cycle OSK (bottom → bottom 50%% → top → top 50%% → hide)\n");
    printf("\tMenu/Guide   Quit\n\n");
    printf("OSK active:\n");
    printf("\tA/LStick     Select key    B   Backspace   Y   Space\n");
    printf("\tStart        Enter         L1/R1 Switch layer\n");
    printf("\tD-pad/LStick Navigate OSK  Vol+/- Page up/down\n\n");
    printf("OSK hidden:\n");
    printf("\tD-pad Up/Down  Scroll history   PgUp/PgDn  Scroll\n");
    exit(0);
}

static int parse_hex_colour(const char *hex, SDL_Color *out) {
    if (!hex || strlen(hex) != 6) return 0;

    char r_s[3] = {hex[0], hex[1], '\0'};
    char g_s[3] = {hex[2], hex[3], '\0'};
    char b_s[3] = {hex[4], hex[5], '\0'};

    char *e;
    errno = 0;

    unsigned long r = strtoul(r_s, &e, 16);
    if (errno || *e || r > 255) return 0;

    unsigned long g = strtoul(g_s, &e, 16);
    if (errno || *e || g > 255) return 0;

    unsigned long b = strtoul(b_s, &e, 16);
    if (errno || *e || b > 255) return 0;

    out->r = (Uint8) r;
    out->g = (Uint8) g;
    out->b = (Uint8) b;
    out->a = 255;

    return 1;
}

int main(int argc, char *argv[]) {
    setlocale(LC_CTYPE, "");

    muTermConfig cfg;
    config_load(&cfg);

    char **child_argv = argv;
    int child_argc = 1;
    int cmd_index = 0;

    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)) print_help(argv[0]);

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "--") == 0) {
            cmd_index = i + 1;
            break;
        } else if ((strcmp(a, "-w") == 0 || strcmp(a, "--width") == 0) && i + 1 < argc) {
            cfg.width = atoi(argv[++i]);
        } else if ((strcmp(a, "-h") == 0 || strcmp(a, "--height") == 0) && i + 1 < argc) {
            cfg.height = atoi(argv[++i]);
        } else if ((strcmp(a, "-s") == 0 || strcmp(a, "--size") == 0) && i + 1 < argc) {
            cfg.font_size = atoi(argv[++i]);
        } else if ((strcmp(a, "-f") == 0 || strcmp(a, "--font") == 0) && i + 1 < argc) {
            snprintf(cfg.font_path, sizeof(cfg.font_path), "%s", argv[++i]);
        } else if ((strcmp(a, "-i") == 0 || strcmp(a, "--image") == 0) && i + 1 < argc) {
            snprintf(cfg.bg_image, sizeof(cfg.bg_image), "%s", argv[++i]);
        } else if ((strcmp(a, "-sb") == 0 || strcmp(a, "--scrollback") == 0) && i + 1 < argc) {
            cfg.scrollback = atoi(argv[++i]);
        } else if ((strcmp(a, "-bg") == 0 || strcmp(a, "--bgcolour") == 0) && i + 1 < argc) {
            if (!parse_hex_colour(argv[++i], &cfg.solid_bg)) {
                fprintf(stderr, "Bad -bg colour\n");
                return 1;
            }
            cfg.use_solid_bg = 1;
        } else if ((strcmp(a, "-fg") == 0 || strcmp(a, "--fgcolour") == 0) && i + 1 < argc) {
            if (!parse_hex_colour(argv[++i], &cfg.solid_fg)) {
                fprintf(stderr, "Bad -fg colour\n");
                return 1;
            }
            cfg.use_solid_fg = 1;
        } else if (strcmp(a, "-ro") == 0 || strcmp(a, "--readonly") == 0) {
            cfg.readonly = 1;
        } else if (strcmp(a, "--zoom") == 0 && i + 1 < argc) {
            cfg.zoom = (float) atof(argv[++i]);
        } else if (strcmp(a, "--rotate") == 0 && i + 1 < argc) {
            cfg.rotate = atoi(argv[++i]);
        } else if (strcmp(a, "--underscan") == 0) {
            cfg.underscan = 1;
        } else if (strcmp(a, "--no-underscan") == 0) {
            cfg.underscan = 0;
        } else if (a[0] != '-') {
            cmd_index = i;
            break;
        }
    }

    if (cmd_index > 0 && cmd_index < argc) {
        child_argv = &argv[cmd_index - 1];
        child_argc = argc - cmd_index + 1;
    }

    if (cfg.scrollback < 1) cfg.scrollback = 1;
    if (cfg.zoom <= 0.0f) cfg.zoom = 1.0f;

    config_dump(&cfg);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GameControllerEventState(1);
    SDL_JoystickEventState(1);

    int num_joy = SDL_NumJoysticks();
    fprintf(stderr, "[SDL] Joysticks: %d\n", num_joy);

    for (int i = 0; i < num_joy; i++) {
        if (SDL_IsGameController(i)) {
            SDL_GameController *gc = SDL_GameControllerOpen(i);
            if (gc) fprintf(stderr, "[SDL] GameController %d: %s\n", i, SDL_GameControllerName(gc));
        } else {
            SDL_Joystick *joy = SDL_JoystickOpen(i);
            if (joy) fprintf(stderr, "[SDL] Joystick %d: %s\n", i, SDL_JoystickName(joy));
        }
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());
        return 1;
    }

    TTF_Font *base = TTF_OpenFont(cfg.font_path, cfg.font_size);
    if (!base) {
        fprintf(stderr, "Cannot open font '%s': %s\n", cfg.font_path, TTF_GetError());
        return 1;
    }

    TTF_SizeUTF8(base, "M", &CELL_WIDTH, &CELL_HEIGHT);

    if (CELL_WIDTH <= 0) CELL_WIDTH = 8;
    if (CELL_HEIGHT <= 0) CELL_HEIGHT = 16;

    TTF_CloseFont(base);

    TTF_Font *fonts[4] = {NULL};
    for (int i = 0; i < 4; i++) {
        fonts[i] = TTF_OpenFont(cfg.font_path, cfg.font_size);

        if (!fonts[i]) {
            fprintf(stderr, "Font[%d]: %s\n", i, TTF_GetError());
            return 1;
        }

        int st = TTF_STYLE_NORMAL;
        if (i & STYLE_BOLD) st |= TTF_STYLE_BOLD;
        if (i & STYLE_UNDERLINE) st |= TTF_STYLE_UNDERLINE;

        TTF_SetFontStyle(fonts[i], st);
    }

    int TERM_COLS = cfg.width / CELL_WIDTH;
    int TERM_ROWS = cfg.height / CELL_HEIGHT;

    if (TERM_COLS < 1) TERM_COLS = 1;
    if (TERM_ROWS < 1) TERM_ROWS = 1;

    if (vt_init(TERM_COLS, TERM_ROWS, cfg.scrollback) < 0) {
        fprintf(stderr, "vt_init failed\n");
        return 1;
    }

    SDL_Color def_fg = {255, 255, 255, 255};
    SDL_Color def_bg = {0, 0, 0, 255};

    render_init(fonts, CELL_WIDTH, CELL_HEIGHT, def_fg, def_bg);

    osk_init(cfg.width, CELL_HEIGHT);

    int pty_fd = -1;
    pid_t child = spawn_pty_child(&pty_fd, child_argc, child_argv, cfg.shell);
    if (child < 0 || pty_fd < 0) {
        fprintf(stderr, "spawn_pty_child failed\n");
        return 1;
    }

    pty_fd_global = pty_fd;

    {
        int fl = fcntl(pty_fd, F_GETFL);
        if (fl >= 0) fcntl(pty_fd, F_SETFL, fl | O_NONBLOCK);
    }

    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = sigchld_handler;
        sa.sa_flags = SA_NOCLDSTOP;
        sigaction(SIGCHLD, &sa, NULL);
    }

    SDL_Window *win = SDL_CreateWindow("Mustard Terminal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       cfg.width, cfg.height, SDL_WINDOW_SHOWN);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }

    SDL_StartTextInput();

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!ren) {
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) {
            fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
            return 1;
        }
    }

    SDL_Texture *render_target = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                                   TERM_COLS * CELL_WIDTH, TERM_ROWS * CELL_HEIGHT);

    if (!render_target) {
        fprintf(stderr, "render_target: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Texture *bg_texture = NULL;
    if (cfg.bg_image[0] && access(cfg.bg_image, R_OK) == 0) {
        SDL_Surface *bgs = IMG_Load(cfg.bg_image);
        if (bgs) {
            bg_texture = SDL_CreateTextureFromSurface(ren, bgs);
            SDL_FreeSurface(bgs);
        }
    }

    SDL_GameController *controller = NULL;
    reopen_controller(&controller);

    int running = 1;
    int shell_dead = 0;
    int vis_rows = TERM_ROWS;

    const Uint32 frame_ms = 33;
    Uint32 last_frame = 0;
    Uint32 last_wait = 0;
    SDL_Event e;

    while (running) {
        while (SDL_PollEvent(&e)) handle_sdl_event(&e, &controller, &running, shell_dead, &vis_rows, cfg.height, cfg.readonly);

        if (osk_hold_tick(SDL_GetTicks())) {
            input_action_t ha = osk_hold_action();
            switch (ha) {
                case INPUT_ACT_PRESS:
                    osk_press_key();
                    break;
                case INPUT_ACT_BACKSPACE:
                    if (!cfg.readonly) {
                        char bs = '\x7F';
                        pty_write(&bs, 1);
                    }
                    break;
                case INPUT_ACT_SPACE:
                    if (!cfg.readonly) {
                        char sp = ' ';
                        pty_write(&sp, 1);
                    }
                    break;
                default:
                    break;
            }
        }

        if (!shell_dead) {
            struct pollfd pfd = {pty_fd, POLLIN, 0};
            poll(&pfd, 1, 0);

            if (pfd.revents & POLLIN) {
                char buf[4096];
                ssize_t n;
                int saw = 0;

                while ((n = read(pty_fd, buf, sizeof(buf))) > 0) {
                    if (vt_feed(buf, (size_t) n)) saw = 1;
                }

                if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                    if (errno == EIO) shell_dead = 1;
                }

                if (saw) vt_scroll_set(0);
            }
        }

        {
            Uint32 now = SDL_GetTicks();
            if (child_exited && !shell_dead) {
                int st;
                if (waitpid(child, &st, WNOHANG) > 0) shell_dead = 1;
            }

            if (!child_exited && now - last_wait >= 250) {
                last_wait = now;
                int st;

                if (waitpid(child, &st, WNOHANG) > 0) shell_dead = 1;
            }
        }

        if (shell_dead) running = 0;

        Uint32 now = SDL_GetTicks();
        if (now - last_frame < frame_ms) {
            SDL_Delay(1);
            continue;
        }

        last_frame = now;

        int need_blink = vt_cursor_visible() && !cfg.readonly && !vt_scroll_offset();

        if (vt_is_dirty() || osk_is_visible() || need_blink) {
            render_screen(ren, render_target, bg_texture, cfg.width, vis_rows, cfg.solid_fg,
                          cfg.use_solid_fg, cfg.use_solid_bg, cfg.solid_bg, cfg.readonly);
            vt_clear_dirty();
        }

        float sw = (float) (TERM_COLS * CELL_WIDTH) * cfg.zoom;
        float sh = (float) (TERM_ROWS * CELL_HEIGHT) * cfg.zoom;

        float us = cfg.underscan ? 16.0f : 0.0f;

        SDL_FRect dest = {
                ((float) cfg.width - sw) / 2.0f + us,
                ((float) cfg.height - sh) / 2.0f + us,
                sw - us * 2.0f, sh - us * 2.0f
        };

        double angle = 0;
        switch (cfg.rotate) {
            case 1:
                angle = 90;
                break;
            case 2:
                angle = 180;
                break;
            case 3:
                angle = 270;
                break;
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopyExF(ren, render_target, NULL, &dest, angle, NULL, SDL_FLIP_NONE);

        if (osk_is_visible()) osk_render(ren, fonts[0], cfg.width, cfg.height);
        SDL_RenderPresent(ren);
    }

    SDL_StopTextInput();

    if (pty_fd >= 0) close(pty_fd);
    if (!shell_dead) waitpid(child, NULL, WNOHANG);

    SDL_DestroyTexture(render_target);
    if (bg_texture) SDL_DestroyTexture(bg_texture);

    render_glyph_cache_clear();
    for (int i = 0; i < 4; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);
    if (controller) SDL_GameControllerClose(controller);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    vt_free();

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
