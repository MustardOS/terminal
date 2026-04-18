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
#include "input.h"

static int CELL_WIDTH = 0;
static int CELL_HEIGHT = 0;

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

    printf("Special Options:\n");
    printf("\t--ignore-muos            Skip MustardOS device, global, and system configs.\n");
    printf("\t                         Utilises built-in defaults and CLI options only.\n");
    printf("\t                         This must be set as the first switch before any others!\n");
    printf("\t                         (user config at $HOME/%s still applies...)\n\n", MUTERM_USR_CONF);

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
    printf("\tL1/R1         Switch layer\n");
    printf("\tL2/R2         Scroll history\n\n");

    printf("OSK Hidden:\n");
    printf("\tD-Pad U/D  Command history\n");
    printf("\tD-Pad L/R  Cursor movement\n");
    printf("\tL2/R2      Scroll history\n\n");

    exit(0);
}

static int parse_hex_colour(const char *hex, SDL_Color *out) {
    if (!hex || strlen(hex) != 6) return 0;

    unsigned int r, g, b;
    if (sscanf(hex, "%2x%2x%2x", &r, &g, &b) != 3) return 0;

    out->r = (Uint8) r;
    out->g = (Uint8) g;
    out->b = (Uint8) b;
    out->a = 255;

    return 1;
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

    TTF_Font *fonts[4] = {NULL};
    for (int i = 0; i < 4; i++) {
        fonts[i] = TTF_OpenFont(cfg.font_path, cfg.font_size);

        if (!fonts[i]) {
            fprintf(stderr, "Font[%d]: %s\n", i, TTF_GetError());
            for (int j = 0; j < i; j++) TTF_CloseFont(fonts[j]);

            TTF_Quit();
            IMG_Quit();
            SDL_Quit();

            return 1;
        }

        int st = TTF_STYLE_NORMAL;
        if (i & STYLE_BOLD) st |= TTF_STYLE_BOLD;
        if (i & STYLE_UNDERLINE) st |= TTF_STYLE_UNDERLINE;

        TTF_SetFontStyle(fonts[i], st);
    }

    int TERM_COLS = term_w / CELL_WIDTH;
    int TERM_ROWS = term_h / CELL_HEIGHT;

    if (TERM_COLS < 1) TERM_COLS = 1;
    if (TERM_ROWS < 1) TERM_ROWS = 1;

    if (vt_init(TERM_COLS, TERM_ROWS, cfg.scrollback) < 0) {
        fprintf(stderr, "vt_init failed\n");
        for (int i = 0; i < 4; i++) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    SDL_Color def_fg = {255, 255, 255, 255};
    SDL_Color def_bg = {0, 0, 0, 255};

    render_init(fonts, CELL_WIDTH, CELL_HEIGHT, def_fg, def_bg);
    osk_init(term_w, CELL_HEIGHT);

    int pty_fd = -1;
    pid_t child = spawn_pty_child(&pty_fd, child_argc, child_argv, cfg.shell);
    if (child < 0 || pty_fd < 0) {
        fprintf(stderr, "spawn_pty_child failed\n");
        vt_free();
        for (int i = 0; i < 4; i++) TTF_CloseFont(fonts[i]);

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

    SDL_Window *win = SDL_CreateWindow("Mustard Terminal", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       cfg.width, cfg.height, SDL_WINDOW_SHOWN);

    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());

        close(pty_fd);
        vt_free();
        for (int i = 0; i < 4; i++) TTF_CloseFont(fonts[i]);

        TTF_Quit();
        IMG_Quit();
        SDL_Quit();

        return 1;
    }

    SDL_StartTextInput();

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!ren) {
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) {
            fprintf(stderr, "SDL_CreateRenderer: %s\n", SDL_GetError());
            SDL_DestroyWindow(win);

            close(pty_fd);
            vt_free();
            for (int i = 0; i < 4; i++) TTF_CloseFont(fonts[i]);

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
        SDL_DestroyWindow(win);

        close(pty_fd);
        vt_free();
        for (int i = 0; i < 4; i++) TTF_CloseFont(fonts[i]);

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

    const Uint32 frame_ms = 33;
    Uint32 last_frame = 0;
    Uint32 last_wait = 0;
    SDL_Event e;

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
            render_screen(ren, render_target, bg_texture, term_w, vis_rows, cfg.solid_fg,
                          cfg.use_solid_fg, cfg.use_solid_bg, cfg.solid_bg, cfg.readonly);
            vt_clear_dirty();

            if (osk_is_visible()) {
                SDL_SetRenderTarget(ren, render_target);
                osk_render(ren, fonts[0], term_w, term_h);
                SDL_SetRenderTarget(ren, NULL);
            }
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
