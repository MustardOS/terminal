#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "config.h"

static char *ltrim(char *s) {
    while (*s && isspace((unsigned char) *s)) s++;
    return s;
}

static void rtrim(char *s) {
    size_t n = strlen(s);
    while (n > 0 && isspace((unsigned char) s[n - 1])) s[--n] = '\0';
}

static int parse_hex_colour(const char *hex, SDL_Color *out) {
    if (!hex) return 0;

    if (*hex == '#') hex++;
    if (strlen(hex) != 6) return 0;

    unsigned int r, g, b;
    if (sscanf(hex, "%2x%2x%2x", &r, &g, &b) != 3) return 0;

    out->r = (Uint8) r;
    out->g = (Uint8) g;
    out->b = (Uint8) b;
    out->a = 255;

    return 1;
}

static void parse_muterm_conf(const char *path, muTermConfig *cfg) {
    FILE *f = fopen(path, "r");
    if (!f) return;

    fprintf(stderr, "[CFG] reading muterm config: %s\n", path);

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        char *p = ltrim(line);
        rtrim(p);

        if (!*p || *p == '#') continue;

        char *eq = strchr(p, '=');
        if (!eq) continue;

        *eq = '\0';

        char *key = p;
        char *val = ltrim(eq + 1);

        rtrim(key);
        rtrim(val);

        if (strcmp(key, "width") == 0) {
            int v = atoi(val);
            if (v > 0) cfg->width = v;
        } else if (strcmp(key, "height") == 0) {
            int v = atoi(val);
            if (v > 0) cfg->height = v;
        } else if (strcmp(key, "font_size") == 0) {
            int v = atoi(val);
            if (v > 0) cfg->font_size = v;
        } else if (strcmp(key, "scrollback") == 0) {
            int v = atoi(val);
            if (v > 0) cfg->scrollback = v;
        } else if (strcmp(key, "readonly") == 0) {
            cfg->readonly = atoi(val) != 0;
        } else if (strcmp(key, "zoom") == 0) {
            float v = (float) atof(val);
            if (v > 0.0f) cfg->zoom = v;
        } else if (strcmp(key, "rotate") == 0) {
            int v = atoi(val);
            if (v >= 0 && v <= 3) cfg->rotate = v;
        } else if (strcmp(key, "underscan") == 0) {
            cfg->underscan = atoi(val) != 0;
        } else if (strcmp(key, "font_path") == 0) {
            if (*val) snprintf(cfg->font_path, sizeof(cfg->font_path), "%s", val);
        } else if (strcmp(key, "bg_image") == 0) {
            snprintf(cfg->bg_image, sizeof(cfg->bg_image), "%s", val);
        } else if (strcmp(key, "shell") == 0) {
            snprintf(cfg->shell, sizeof(cfg->shell), "%s", val);
        } else if (strcmp(key, "bg_colour") == 0) {
            SDL_Color c = {0, 0, 0, 255};
            if (*val && parse_hex_colour(val, &c)) {
                cfg->solid_bg = c;
                cfg->use_solid_bg = 1;
            }
        } else if (strcmp(key, "fg_colour") == 0) {
            SDL_Color c = {255, 255, 255, 255};
            if (*val && parse_hex_colour(val, &c)) {
                cfg->solid_fg = c;
                cfg->use_solid_fg = 1;
            }
        }
    }

    fclose(f);
}

static int read_muos_file(const char *base, const char *rel, char *buf) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", base, rel);

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    size_t n = fread(buf, 1, 255, f);
    fclose(f);

    buf[n] = '\0';
    while (n > 0 && (unsigned char) buf[n - 1] <= ' ') buf[--n] = '\0';

    fprintf(stderr, "[CFG] muOS %s = \"%s\"\n", path, buf);
    return n > 0;
}

static void read_muos_device_config(const char *base, muTermConfig *cfg) {
    char val[256];

    if (read_muos_file(base, "screen/width", val)) {
        int v = atoi(val);
        if (v > 0) cfg->width = v;
    }

    if (read_muos_file(base, "screen/height", val)) {
        int v = atoi(val);
        if (v > 0) cfg->height = v;
    }

    if (read_muos_file(base, "screen/zoom", val)) {
        float v = (float) atof(val);
        if (v > 0.0f) cfg->zoom = v;
    }

    if (read_muos_file(base, "screen/rotate", val)) {
        int v = atoi(val);
        if (v >= 0 && v <= 3) cfg->rotate = v;
    }

    if (read_muos_file(base, "board/name", val)) {
        if (strcasecmp(val, "tui-brick") == 0) cfg->font_size = 28;
    }
}

static void read_muos_global_config(const char *base, muTermConfig *cfg) {
    char val[256];
    if (read_muos_file(base, "settings/hdmi/scan", val)) cfg->underscan = (atoi(val) == 1);
}

void config_load(muTermConfig *cfg, int ignore_muos) {
    memset(cfg, 0, sizeof(*cfg));

    cfg->width = MUTERM_DEFAULT_WIDTH;
    cfg->height = MUTERM_DEFAULT_HEIGHT;
    cfg->font_size = MUTERM_DEFAULT_FONT_SIZE;
    cfg->scrollback = MUTERM_DEFAULT_SCROLLBACK;

    cfg->zoom = 1.0f;
    cfg->ignore_muos = ignore_muos;

    cfg->solid_bg = (SDL_Color) {0, 0, 0, 255};
    cfg->solid_fg = (SDL_Color) {255, 255, 255, 255};

    snprintf(cfg->font_path, sizeof(cfg->font_path), "%s", MUTERM_DEFAULT_FONT_PATH);

    if (!ignore_muos) {
        read_muos_device_config(MUOS_DEVICE_CONFIG, cfg);
        read_muos_global_config(MUOS_GLOBAL_CONFIG, cfg);

        parse_muterm_conf(MUTERM_SYS_CONF, cfg);
    } else {
        fprintf(stderr, "[CFG] --ignore-muos: skipping muOS device, global, and system configs\n");
    }

    const char *home = getenv("HOME");
    if (home && *home) {
        char user_conf[1024];
        snprintf(user_conf, sizeof(user_conf), "%s/%s", home, MUTERM_USR_CONF);
        parse_muterm_conf(user_conf, cfg);
    }
}

void config_dump(const muTermConfig *cfg) {
    fprintf(stderr, "[CFG] width=%d height=%d font=%s size=%d\n", cfg->width, cfg->height, cfg->font_path, cfg->font_size);

    fprintf(stderr, "[CFG] scroll=%d zoom=%.2f rotate=%d underscan=%d readonly=%d ignore_muos=%d\n",
            cfg->scrollback, cfg->zoom, cfg->rotate, cfg->underscan, cfg->readonly, cfg->ignore_muos);

    if (cfg->use_solid_bg) fprintf(stderr, "[CFG] solid_bg=#%02X%02X%02X\n", cfg->solid_bg.r, cfg->solid_bg.g, cfg->solid_bg.b);
    if (cfg->use_solid_fg) fprintf(stderr, "[CFG] solid_fg=#%02X%02X%02X\n", cfg->solid_fg.r, cfg->solid_fg.g, cfg->solid_fg.b);

    if (cfg->bg_image[0]) fprintf(stderr, "[CFG] bg_image=%s\n", cfg->bg_image);

    if (cfg->shell[0]) fprintf(stderr, "[CFG] shell=%s\n", cfg->shell);
}
