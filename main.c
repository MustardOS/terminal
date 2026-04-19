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
#include <ctype.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include "config.h"
#include "vt.h"
#include "render.h"
#include "osk.h"
#include "input.h"

static int CELL_WIDTH = 0;
static int CELL_HEIGHT = 0;

static volatile sig_atomic_t child_exited = 0;

static SDL_Window *g_win = NULL;

static void sigchld_handler(int s) {
    (void) s;
    child_exited = 1;
}

static void on_title_change(const char *title, void *userdata) {
    (void) userdata;
    if (g_win && title && *title) SDL_SetWindowTitle(g_win, title);
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

        char cols_buf[16], rows_buf[16];
        snprintf(cols_buf, sizeof(cols_buf), "%d", vt_cols());
        snprintf(rows_buf, sizeof(rows_buf), "%d", vt_rows());
        setenv("COLUMNS", cols_buf, 1);
        setenv("LINES", rows_buf, 1);

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

static void print_help(const char *name) {
    printf("muTerm – Portable Virtual Terminal\n\n");
    printf("Usage:\n\t%s [options] [-- command [args...]]\n\n", name);

    printf("Options:\n");
    printf("\t-w, --width  <px>              Window width  (default: from config / %d)\n", MUTERM_DEFAULT_WIDTH);
    printf("\t-h, --height <px>              Window height (default: from config / %d)\n", MUTERM_DEFAULT_HEIGHT);
    printf("\t-s, --size   <pt>              Font size     (default: from config / %d)\n", MUTERM_DEFAULT_FONT_SIZE);
    printf("\t-f, --font   <path>            Font file     (default: from config / built-in)\n");
    printf("\t    --font-italic <path>       Italic font variant (falls back to SDL_ttf style)\n");
    printf("\t    --font-bold   <path>       Bold font variant   (falls back to SDL_ttf style)\n");
    printf("\t    --font-bold-italic <path>  Bold+italic variant (falls back to SDL_ttf style)\n");
    printf("\t-i, --image  <file>            Background PNG\n");
    printf("\t-sb,--scrollback <n>           Scrollback lines (default: %d)\n", MUTERM_DEFAULT_SCROLLBACK);
    printf("\t    --sb-path <file>           Scrollback persistence file (default: %s)\n", MUTERM_DEFAULT_SB_PATH);
    printf("\t    --no-sb-persist            Disable scrollback save/load\n");
    printf("\t-bg,--bgcolour RRGGBB          Solid background colour\n");
    printf("\t-fg,--fgcolour RRGGBB          Solid foreground colour (overrides ANSI)\n");
    printf("\t-ro,--readonly                 View-only: display PTY output, no input\n");
    printf("\t    --zoom <factor>            Display zoom  (e.g. 1.5)\n");
    printf("\t    --rotate <0-3>             Rotate display (0=none 1=90 2=180 3=270)\n");
    printf("\t    --underscan                Apply HDMI underscan (16 px)\n");
    printf("\t    --no-underscan             Disable underscan\n");
    printf("\t    --osk-layout <file>        Extra OSK layer file\n");
    printf("\t    --key-delay  <ms>          OSK key hold delay before repeat (default: %d ms)\n", MUTERM_DEFAULT_KEY_DELAY);
    printf("\t    --key-rate   <ms>          OSK key repeat interval          (default: %d ms)\n", MUTERM_DEFAULT_KEY_RATE);
    printf("\t    --dpad-delay <ms>          D-Pad hold delay before repeat   (default: %d ms)\n", MUTERM_DEFAULT_DPAD_DELAY);
    printf("\t    --dpad-rate  <ms>          D-Pad repeat interval            (default: %d ms)\n", MUTERM_DEFAULT_DPAD_RATE);
    printf("\t    --force_redraw             Force full redraw every frame (will utilise more CPU cycles)\n\n");

    printf("Special Options:\n");
    printf("\t--ignore-muos  Skip MustardOS device, global, and system configs.\n");
    printf("\t               Utilises built-in defaults and CLI options only.\n");
    printf("\t               This must be set as the first switch before any others!\n");
    printf("\t               (user config at $HOME/%s still applies...)\n\n", MUTERM_USR_CONF);

    printf("Config Files: (lower entries override higher)\n");
    printf("\t%s\n\t%s\n\t%s\n\t$HOME/%s\n\n", MUOS_DEVICE_CONFIG, MUOS_GLOBAL_CONFIG, MUTERM_SYS_CONF, MUTERM_USR_CONF);

    printf("Controls:\n");
    printf("\tSelect      Cycle OSK (bottom → bottom 50%% → top → top 50%% → hide)\n");
    printf("\tMenu/Guide  Quit\n\n");

    printf("OSK Visible:\n");
    printf("\tD-Pad/LStick  Navigate OSK\n");
    printf("\tA/LStick      Press key\n");
    printf("\tB             Backspace\n");
    printf("\tY             Space\n");
    printf("\tStart         Enter\n");
    printf("\tL1/R1         Switch layer (cycles through all loaded layers)\n");
    printf("\tL2/R2         Scroll history\n\n");

    printf("OSK Hidden:\n");
    printf("\tD-Pad U/D  Command history\n");
    printf("\tD-Pad L/R  Cursor movement\n");
    printf("\tL2/R2      Scroll history\n\n");

    exit(0);
}

static int parse_hex_colour(const char *hex, SDL_Color *out) {
    if (!hex) return 0;

    while (*hex && isspace((unsigned char) *hex)) hex++;
    if (*hex == '#') hex++;

    while (*hex && isspace((unsigned char) *hex)) hex++;
    if (strlen(hex) < 6) return 0;

    unsigned int r, g, b;
    if (sscanf(hex, "%2x%2x%2x", &r, &g, &b) != 3) return 0;

    out->r = (Uint8) r;
    out->g = (Uint8) g;
    out->b = (Uint8) b;
    out->a = 255;

    return 1;
}

static TTF_Font *open_font_slot(const muTermConfig *cfg, int slot) {
    int is_bold = (slot & 1) != 0;
    int is_underline = (slot & 2) != 0;
    int is_italic = (slot & 4) != 0;

    const char *explicit_path = NULL;
    if (is_bold && is_italic && cfg->font_path_bold_italic[0]) {
        explicit_path = cfg->font_path_bold_italic;
    } else if (is_italic && !is_bold && cfg->font_path_italic[0]) {
        explicit_path = cfg->font_path_italic;
    } else if (is_bold && !is_italic && cfg->font_path_bold[0]) {
        explicit_path = cfg->font_path_bold;
    }

    const char *path = explicit_path ? explicit_path : cfg->font_path;

    TTF_Font *font = TTF_OpenFont(path, cfg->font_size);
    if (!font) {
        fprintf(stderr, "Font[%d] '%s': %s\n", slot, path, TTF_GetError());
        return NULL;
    }

    if (!explicit_path) {
        int st = TTF_STYLE_NORMAL;

        if (is_bold) st |= TTF_STYLE_BOLD;
        if (is_underline) st |= TTF_STYLE_UNDERLINE;
        if (is_italic) st |= TTF_STYLE_ITALIC;

        if (st != TTF_STYLE_NORMAL) TTF_SetFontStyle(font, st);
    }

    return font;
}

int main(int argc, char *argv[]) {
    setlocale(LC_CTYPE, "");

    int ignore_muos = 0;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--") == 0) break;
        if (strcmp(argv[i], "--ignore-muos") == 0) {
            ignore_muos = 1;
            break;
        }
    }

    muTermConfig cfg;
    config_load(&cfg, ignore_muos);

    char **child_argv = argv;
    int child_argc = 1;
    int cmd_index = 0;
    int no_sb_persist = 0;

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
        } else if (strcmp(a, "--font-italic") == 0 && i + 1 < argc) {
            snprintf(cfg.font_path_italic, sizeof(cfg.font_path_italic), "%s", argv[++i]);
        } else if (strcmp(a, "--font-bold") == 0 && i + 1 < argc) {
            snprintf(cfg.font_path_bold, sizeof(cfg.font_path_bold), "%s", argv[++i]);
        } else if (strcmp(a, "--font-bold-italic") == 0 && i + 1 < argc) {
            snprintf(cfg.font_path_bold_italic, sizeof(cfg.font_path_bold_italic), "%s", argv[++i]);
        } else if ((strcmp(a, "-i") == 0 || strcmp(a, "--image") == 0) && i + 1 < argc) {
            snprintf(cfg.bg_image, sizeof(cfg.bg_image), "%s", argv[++i]);
        } else if ((strcmp(a, "-sb") == 0 || strcmp(a, "--scrollback") == 0) && i + 1 < argc) {
            cfg.scrollback = atoi(argv[++i]);
        } else if (strcmp(a, "--sb-path") == 0 && i + 1 < argc) {
            snprintf(cfg.scrollback_path, sizeof(cfg.scrollback_path), "%s", argv[++i]);
        } else if (strcmp(a, "--no-sb-persist") == 0) {
            no_sb_persist = 1;
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
        } else if (strcmp(a, "--osk-layout") == 0 && i + 1 < argc) {
            snprintf(cfg.osk_layout_path, sizeof(cfg.osk_layout_path), "%s", argv[++i]);
        } else if (strcmp(a, "--key-delay") == 0 && i + 1 < argc) {
            cfg.key_repeat_delay = atoi(argv[++i]);
        } else if (strcmp(a, "--key-rate") == 0 && i + 1 < argc) {
            cfg.key_repeat_rate = atoi(argv[++i]);
        } else if (strcmp(a, "--dpad-delay") == 0 && i + 1 < argc) {
            cfg.dpad_repeat_delay = atoi(argv[++i]);
        } else if (strcmp(a, "--dpad-rate") == 0 && i + 1 < argc) {
            cfg.dpad_repeat_rate = atoi(argv[++i]);
        } else if (strcmp(a, "--force_redraw") == 0) {
            cfg.force_redraw = 1;
        } else if (strcmp(a, "--ignore-muos") == 0) {
            // Handled in pre-scan above...
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

    int term_w = cfg.width;
    int term_h = cfg.height;

    if (cfg.rotate == 1 || cfg.rotate == 3) {
        term_w = cfg.height;
        term_h = cfg.width;
    }

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
            if (gc) {
                fprintf(stderr, "[SDL] GameController %d: %s\n", i, SDL_GameControllerName(gc));
                SDL_GameControllerClose(gc);
            }
        } else {
            SDL_Joystick *joy = SDL_JoystickOpen(i);
            if (joy) {
                fprintf(stderr, "[SDL] Joystick %d: %s\n", i, SDL_JoystickName(joy));
                SDL_JoystickClose(joy);
            }
        }
    }

    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());

        SDL_Quit();

        return 1;
    }

    if (TTF_Init() != 0) {
        fprintf(stderr, "TTF_Init: %s\n", TTF_GetError());

        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    TTF_Font *base = TTF_OpenFont(cfg.font_path, cfg.font_size);
    if (!base) {
        fprintf(stderr, "Cannot open font '%s': %s\n", cfg.font_path, TTF_GetError());

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    TTF_SizeUTF8(base, "M", &CELL_WIDTH, &CELL_HEIGHT);

    if (CELL_WIDTH <= 0) CELL_WIDTH = 8;
    if (CELL_HEIGHT <= 0) CELL_HEIGHT = 16;

    TTF_CloseFont(base);

    TTF_Font *fonts[8] = {NULL};
    for (int i = 0; i < 8; i++) {
        fonts[i] = open_font_slot(&cfg, i);
        if (!fonts[i]) fprintf(stderr, "[FONT] slot %d unavailable, will use slot 0\n", i);
    }

    if (!fonts[0]) {
        fprintf(stderr, "Base font (slot 0) failed — cannot continue\n");
        for (int i = 1; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    int TERM_COLS = term_w / CELL_WIDTH;
    int TERM_ROWS = term_h / CELL_HEIGHT;

    if (TERM_COLS < 1) TERM_COLS = 1;
    if (TERM_ROWS < 1) TERM_ROWS = 1;

    if (vt_init(TERM_COLS, TERM_ROWS, cfg.scrollback) < 0) {
        fprintf(stderr, "vt_init failed\n");
        for (int i = 0; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    if (!no_sb_persist && cfg.scrollback_path[0]) vt_scrollback_load(cfg.scrollback_path);

    SDL_Color def_fg = {255, 255, 255, 255};
    SDL_Color def_bg = {0, 0, 0, 255};

    render_init(fonts, CELL_WIDTH, CELL_HEIGHT, def_fg, def_bg);
    osk_init(term_w, CELL_HEIGHT);

    osk_set_repeat(cfg.key_repeat_delay, cfg.key_repeat_rate);
    input_set_dpad_repeat(cfg.dpad_repeat_delay, cfg.dpad_repeat_rate);

    if (cfg.osk_layout_path[0]) osk_load_layout(cfg.osk_layout_path);

    int pty_fd = -1;
    pid_t child = spawn_pty_child(&pty_fd, child_argc, child_argv, cfg.shell);
    if (child < 0 || pty_fd < 0) {
        fprintf(stderr, "spawn_pty_child failed\n");
        osk_free();
        vt_free();
        for (int i = 0; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    input_set_pty_fd(pty_fd);

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

        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, NULL);
    }

    g_win = SDL_CreateWindow("Mustard Terminal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                             cfg.width, cfg.height, SDL_WINDOW_SHOWN);

    if (!g_win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());

        close(pty_fd);
        osk_free();
        vt_free();
        for (int i = 0; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    vt_set_title_callback(on_title_change, NULL);

    SDL_StartTextInput();

    SDL_Renderer *ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!ren) {
        ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) {
            fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
            SDL_DestroyWindow(g_win);

            close(pty_fd);
            osk_free();
            vt_free();
            for (int i = 0; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);

            TTF_Quit();
            IMG_Quit();
            SDL_Quit();

            return 1;
        }
    }

    SDL_Texture *render_target = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
                                                   TERM_COLS * CELL_WIDTH, TERM_ROWS * CELL_HEIGHT);

    if (!render_target) {
        fprintf(stderr, "render_target: %s\n", SDL_GetError());

        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(g_win);

        close(pty_fd);
        osk_free();
        vt_free();
        for (int i = 0; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

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
    input_reopen_controller(&controller);

    int running = 1;
    int shell_dead = 0;
    int vis_rows = TERM_ROWS;
    int osk_was_visible = OSK_STATE_HIDDEN;

    const Uint32 frame_ms = 33;
    Uint32 last_frame = 0;
    Uint32 last_wait = 0;
    SDL_Event e;

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

#define FADE_STEP(alpha) do { \
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255); \
    SDL_RenderClear(ren); \
    SDL_RenderCopyExF(ren, render_target, NULL, &dest, angle, NULL, SDL_FLIP_NONE); \
    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND); \
    SDL_SetRenderDrawColor(ren, 0, 0, 0, (Uint8) (alpha)); \
    SDL_RenderFillRect(ren, NULL); \
    SDL_RenderPresent(ren); \
    SDL_Delay(frame_ms); \
} while (0)

    int fade_in_done = 0;

    while (running) {
        while (SDL_PollEvent(&e)) input_handle_sdl_event(&e, &controller, &running, shell_dead, &vis_rows, term_h, cfg.readonly);

        input_dpad_tick(SDL_GetTicks());

        if (osk_hold_tick(SDL_GetTicks())) {
            switch (osk_hold_action()) {
                case INPUT_ACT_PRESS:
                    osk_press_key();
                    break;
                case INPUT_ACT_BACKSPACE:
                    if (!cfg.readonly) {
                        char bs = '\x7F';
                        write(pty_fd, &bs, 1);
                    }
                    break;
                case INPUT_ACT_SPACE:
                    if (!cfg.readonly) {
                        char sp = ' ';
                        write(pty_fd, &sp, 1);
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
                static char pty_batch[65536];
                size_t total = 0;
                ssize_t n = 0;

                while (total < sizeof(pty_batch) && (n = read(pty_fd, pty_batch + total, sizeof(pty_batch) - total)) > 0) {
                    total += (size_t) n;
                }

                if (n < 0 && errno == EIO) shell_dead = 1;
                if (total > 0 && vt_feed(pty_batch, total)) vt_scroll_set(0);
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
            SDL_Delay(frame_ms - (now - last_frame));
            continue;
        }

        last_frame = now;

        int need_blink = vt_cursor_visible() && !cfg.readonly && !vt_scroll_offset();

        int osk_now_state = osk_get_state();
        if (osk_now_state != osk_was_visible) {
            vt_mark_all_rows_dirty();
            osk_was_visible = osk_now_state;
        }

        int osk_is_trans = (osk_now_state == OSK_STATE_BOTTOM_TRANS || osk_now_state == OSK_STATE_TOP_TRANS);
        int force_live_redraw = cfg.force_redraw || osk_is_trans || vt_using_alt_screen();
        if (force_live_redraw) vt_mark_all_rows_dirty();

        int had_content = !fade_in_done && vt_is_dirty();

        if (vt_is_dirty()) {
            render_screen(ren, render_target, bg_texture, term_w, vis_rows, cfg.solid_fg,
                          cfg.use_solid_fg, cfg.use_solid_bg, cfg.solid_bg, cfg.readonly);
            vt_clear_dirty();

            if (osk_is_visible()) {
                SDL_SetRenderTarget(ren, render_target);
                osk_render(ren, fonts[0], term_w, term_h);
                SDL_SetRenderTarget(ren, NULL);
            }
        } else if (osk_is_visible()) {
            if (need_blink) render_cursor_blink(ren, render_target, cfg.readonly);

            SDL_SetRenderTarget(ren, render_target);
            osk_render(ren, fonts[0], term_w, term_h);
            SDL_SetRenderTarget(ren, NULL);
        } else if (need_blink) {
            render_cursor_blink(ren, render_target, cfg.readonly);
        }

        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);
        SDL_RenderCopyExF(ren, render_target, NULL, &dest, angle, NULL, SDL_FLIP_NONE);

        if (!fade_in_done && had_content) {
            FADE_STEP(255);
            FADE_STEP(200);
            FADE_STEP(140);
            FADE_STEP(80);
            FADE_STEP(30);
            FADE_STEP(0);

            fade_in_done = 1;
        } else {
            SDL_RenderPresent(ren);
        }
    }

    {
        FADE_STEP(0);
        FADE_STEP(30);
        FADE_STEP(80);
        FADE_STEP(140);
        FADE_STEP(200);
        FADE_STEP(255);
    }

#undef FADE_STEP

    SDL_StopTextInput();

    if (!no_sb_persist && cfg.scrollback_path[0]) vt_scrollback_save(cfg.scrollback_path);

    if (pty_fd >= 0) close(pty_fd);
    if (!shell_dead) waitpid(child, NULL, WNOHANG);

    SDL_DestroyTexture(render_target);
    if (bg_texture) SDL_DestroyTexture(bg_texture);

    render_glyph_cache_clear();
    for (int i = 0; i < 8; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);
    if (controller) SDL_GameControllerClose(controller);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(g_win);

    osk_free();
    vt_free();

    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return 0;
}
