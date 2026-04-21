#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "osk.h"
#include "vt.h"

typedef struct {
    const char *label;
    const char *send;
    int width;
} OskKey;

typedef struct {
    const OskKey *keys;
    int count;
} OskRow;

#define OSK_BUILTIN_LAYERS 3
#define OSK_LAYERS_MAX     16
#define OSK_ROWS           5
#define OSK_MAX_COLS       12
#define OSK_KEY_PAD        2
#define OSK_MARGIN         4

static Uint32 g_key_repeat_delay = 350;
static Uint32 g_key_repeat_rate = 70;

static const OskKey OSK_EMPTY = {NULL, NULL, 0};

static const OskKey row_lower_0[] = {
        {"1", "1", 1},
        {"2", "2", 1},
        {"3", "3", 1},
        {"4", "4", 1},
        {"5", "5", 1},
        {"6", "6", 1},
        {"7", "7", 1},
        {"8", "8", 1},
        {"9", "9", 1},
        {"0", "0", 1},
        {"-", "-", 1},
        {"=", "=", 1}};

static const OskKey row_lower_1[] = {
        {"q", "q", 1},
        {"w", "w", 1},
        {"e", "e", 1},
        {"r", "r", 1},
        {"t", "t", 1},
        {"y", "y", 1},
        {"u", "u", 1},
        {"i", "i", 1},
        {"o", "o", 1},
        {"p", "p", 1},
        {"[", "[", 1},
        {"]", "]", 1}};

static const OskKey row_lower_2[] = {
        {"a",  "a",  1},
        {"s",  "s",  1},
        {"d",  "d",  1},
        {"f",  "f",  1},
        {"g",  "g",  1},
        {"h",  "h",  1},
        {"j",  "j",  1},
        {"k",  "k",  1},
        {"l",  "l",  1},
        {";",  ";",  1},
        {"'",  "'",  1},
        {"\\", "\\", 1}};

static const OskKey row_lower_3[] = {
        {"z", "z", 1},
        {"x", "x", 1},
        {"c", "c", 1},
        {"v", "v", 1},
        {"b", "b", 1},
        {"n", "n", 1},
        {"m", "m", 1},
        {",", ",", 1},
        {".", ".", 1},
        {"/", "/", 1},
        {"$", "$", 1},
        {"`", "`", 1}};

static const OskKey row_upper_0[] = {
        {"!", "!", 1},
        {"@", "@", 1},
        {"#", "#", 1},
        {"$", "$", 1},
        {"%", "%", 1},
        {"^", "^", 1},
        {"&", "&", 1},
        {"*", "*", 1},
        {"(", "(", 1},
        {")", ")", 1},
        {"_", "_", 1},
        {"+", "+", 1}};

static const OskKey row_upper_1[] = {
        {"Q", "Q", 1},
        {"W", "W", 1},
        {"E", "E", 1},
        {"R", "R", 1},
        {"T", "T", 1},
        {"Y", "Y", 1},
        {"U", "U", 1},
        {"I", "I", 1},
        {"O", "O", 1},
        {"P", "P", 1},
        {"{", "{", 1},
        {"}", "}", 1}};

static const OskKey row_upper_2[] = {
        {"A",  "A",  1},
        {"S",  "S",  1},
        {"D",  "D",  1},
        {"F",  "F",  1},
        {"G",  "G",  1},
        {"H",  "H",  1},
        {"J",  "J",  1},
        {"K",  "K",  1},
        {"L",  "L",  1},
        {":",  ":",  1},
        {"\"", "\"", 1},
        {"|",  "|",  1}};

static const OskKey row_upper_3[] = {
        {"Z", "Z", 1},
        {"X", "X", 1},
        {"C", "C", 1},
        {"V", "V", 1},
        {"B", "B", 1},
        {"N", "N", 1},
        {"M", "M", 1},
        {"<", "<", 1},
        {">", ">", 1},
        {"?", "?", 1},
        {"~", "~", 1},
        {"@", "@", 1}};

static const OskKey row_ctrl_0[] = {
        {"F1",  "\x1BOP",   1},
        {"F2",  "\x1BOQ",   1},
        {"F3",  "\x1BOR",   1},
        {"F4",  "\x1BOS",   1},
        {"F5",  "\x1B[15~", 1},
        {"F6",  "\x1B[17~", 1},
        {"F7",  "\x1B[18~", 1},
        {"F8",  "\x1B[19~", 1},
        {"F9",  "\x1B[20~", 1},
        {"F10", "\x1B[21~", 1},
        {"F11", "\x1B[23~", 1},
        {"F12", "\x1B[24~", 1}};

static const OskKey row_ctrl_1[] = {
        {"C-a", "\x01", 1},
        {"C-b", "\x02", 1},
        {"C-c", "\x03", 1},
        {"C-d", "\x04", 1},
        {"C-e", "\x05", 1},
        {"C-f", "\x06", 1},
        {"C-g", "\x07", 1},
        {"C-h", "\x08", 1},
        {"C-i", "\x09", 1},
        {"C-j", "\x0A", 1},
        {"C-k", "\x0B", 1},
        {"C-l", "\x0C", 1}};

static const OskKey row_ctrl_2[] = {
        {"C-m", "\x0D", 1},
        {"C-n", "\x0E", 1},
        {"C-o", "\x0F", 1},
        {"C-p", "\x10", 1},
        {"C-q", "\x11", 1},
        {"C-r", "\x12", 1},
        {"C-s", "\x13", 1},
        {"C-t", "\x14", 1},
        {"C-u", "\x15", 1},
        {"C-v", "\x16", 1},
        {"C-w", "\x17", 1},
        {"C-x", "\x18", 1}};

static const OskKey row_ctrl_3[] = {
        {"C-y",  "\x19", 1},
        {"C-z",  "\x1A", 1},
        {"C-[",  "\x1B", 1},
        {"C-\\", "\x1C", 1},
        {"C-]",  "\x1D", 1},
        {"C-^",  "\x1E", 1},
        {"C-_",  "\x1F", 1}};

static const OskKey row_nav[] = {
        {"Tab",   "\t",     1},
        {"Esc",   "\x1B",   1},
        {"Ctrl",  "",       1},
        {"Alt",   "",       1},
        {"Up",    "\x1B[A", 2},
        {"Down",  "\x1B[B", 2},
        {"Left",  "\x1B[D", 2},
        {"Right", "\x1B[C", 2}};

#define ROW(arr) {arr, (int)(sizeof(arr)/sizeof(OskKey))}

static const OskRow layer_lower_rows[OSK_ROWS] = {ROW(row_lower_0), ROW(row_lower_1), ROW(row_lower_2), ROW(row_lower_3), ROW(row_nav)};
static const OskRow layer_upper_rows[OSK_ROWS] = {ROW(row_upper_0), ROW(row_upper_1), ROW(row_upper_2), ROW(row_upper_3), ROW(row_nav)};
static const OskRow layer_ctrl_rows[OSK_ROWS] = {ROW(row_ctrl_0), ROW(row_ctrl_1), ROW(row_ctrl_2), ROW(row_ctrl_3), ROW(row_nav)};

static OskKey layer_lower[OSK_ROWS][OSK_MAX_COLS];
static OskKey layer_upper[OSK_ROWS][OSK_MAX_COLS];
static OskKey layer_ctrl[OSK_ROWS][OSK_MAX_COLS];

typedef struct {
    OskKey grid[OSK_ROWS][OSK_MAX_COLS];
    char name[32];
} DynLayer;

static DynLayer *g_dyn_layers[OSK_LAYERS_MAX - OSK_BUILTIN_LAYERS];
static int g_dyn_layer_count = 0;

static OskKey (*osk_layers[OSK_LAYERS_MAX])[OSK_MAX_COLS];
static char osk_layer_names[OSK_LAYERS_MAX][32];
static int osk_layer_count = OSK_BUILTIN_LAYERS;

static int osk_state = OSK_STATE_HIDDEN;
static int osk_visible = 0;
static int osk_layer = 0;
static int osk_sel_row = 0;
static int osk_sel_col = 0;
static int osk_ctrl = 0;
static int osk_alt = 0;

static int osk_key_w = 0;
static int osk_key_h = 0;
static int osk_height = 0;
static int g_cell_h = 16;

static int osk_hold_active = 0;
static input_action_t g_hold_action = INPUT_ACT_NONE;
static Uint32 osk_hold_press_t = 0;
static Uint32 osk_hold_last_rep = 0;

static int axis_x_state = 0;
static int axis_y_state = 0;

static int g_pty_fd = -1;

static void pty_write(int fd, const char *data, int len) {
    if (fd < 0 || len <= 0) return;

    ssize_t r = write(fd, data, (size_t) len);
    (void) r;
}

static void build_layer(const OskRow *rows, OskKey out[OSK_ROWS][OSK_MAX_COLS]) {
    for (int r = 0; r < OSK_ROWS; r++) {
        int c = 0;
        if (rows[r].keys) {
            for (int i = 0; i < rows[r].count && c < OSK_MAX_COLS; i++) {
                out[r][c++] = rows[r].keys[i];
            }
        }

        while (c < OSK_MAX_COLS) out[r][c++] = OSK_EMPTY;
    }
}

static int row_len(int layer, int row) {
    const OskKey *r = osk_layers[layer][row];
    int n = 0;

    for (int i = 0; i < OSK_MAX_COLS; i++) {
        if (!r[i].label) break;
        n++;
    }

    return n;
}

void osk_init(int screen_w, int cell_h) {
    build_layer(layer_lower_rows, layer_lower);
    build_layer(layer_upper_rows, layer_upper);
    build_layer(layer_ctrl_rows, layer_ctrl);

    osk_layers[0] = layer_lower;
    osk_layers[1] = layer_upper;
    osk_layers[2] = layer_ctrl;

    snprintf(osk_layer_names[0], sizeof(osk_layer_names[0]), "abc");
    snprintf(osk_layer_names[1], sizeof(osk_layer_names[1]), "ABC");
    snprintf(osk_layer_names[2], sizeof(osk_layer_names[2]), "Ctrl");

    osk_layer_count = OSK_BUILTIN_LAYERS;

    g_cell_h = cell_h;

    osk_key_h = cell_h + 6;
    osk_key_w = (screen_w - 2 * OSK_MARGIN) / OSK_MAX_COLS - OSK_KEY_PAD;
    osk_height = OSK_ROWS * (osk_key_h + OSK_KEY_PAD) + OSK_MARGIN * 2 + osk_key_h;

    fprintf(stderr, "[OSK] METRICS key_w=%d key_h=%d height=%d screen_w=%d\n", osk_key_w, osk_key_h, osk_height, screen_w);
}

static int parse_send(const char *src, char *out) {
    int len = 0;

    while (*src && len < 64 - 1) {
        if (src[0] == '\\' && src[1]) {
            src++;
            switch (*src) {
                case 't':
                    out[len++] = '\t';
                    break;
                case 'r':
                    out[len++] = '\r';
                    break;
                case 'n':
                    out[len++] = '\n';
                    break;
                case '\\':
                    out[len++] = '\\';
                    break;
                case 'x':
                case 'X':
                    if (src[1] && src[2]) {
                        char hex[3] = {src[1], src[2], '\0'};
                        out[len++] = (char) strtol(hex, NULL, 16);
                        src += 2;
                    }
                    break;
                default:
                    out[len++] = *src;
                    break;
            }
            src++;
        } else {
            out[len++] = *src++;
        }
    }

    out[len] = '\0';
    return len;
}

int osk_load_layout(const char *path) {
    if (!path || !*path) return 0;

    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[OSK] cannot open layout: %s\n", path);
        return -1;
    }

    fprintf(stderr, "[OSK] loading layout: %s\n", path);

    int loaded = 0;
    DynLayer *cur = NULL;
    int cur_row = 0;

    char line[512];
    while (fgets(line, sizeof(line), f)) {
        size_t ln = strlen(line);
        while (ln > 0 && ((unsigned char) line[ln - 1] <= ' ')) line[--ln] = '\0';

        char *p = line;
        while (*p && (unsigned char) *p <= ' ') p++;

        if (!*p || *p == '#') continue;

        if (*p == '[') {
            char name[32] = {0};
            const char *end = strchr(p + 1, ']');
            if (!end) continue;

            size_t nlen = (size_t) (end - (p + 1));
            if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
            memcpy(name, p + 1, nlen);
            name[nlen] = '\0';

            if (osk_layer_count >= OSK_LAYERS_MAX) {
                fprintf(stderr, "[OSK] max layers reached, skipping '%s'\n", name);
                cur = NULL;
                continue;
            }

            cur = (DynLayer *) calloc(1, sizeof(DynLayer));
            if (!cur) continue;

            snprintf(cur->name, sizeof(cur->name), "%s", name);

            for (int r = 0; r < OSK_ROWS; r++) {
                for (int c = 0; c < OSK_MAX_COLS; c++) {
                    cur->grid[r][c] = OSK_EMPTY;
                }
            }

            cur_row = 0;

            int slot = osk_layer_count;
            g_dyn_layers[g_dyn_layer_count++] = cur;
            osk_layers[slot] = cur->grid;
            snprintf(osk_layer_names[slot], sizeof(osk_layer_names[slot]), "%s", name);
            osk_layer_count++;
            loaded++;

            fprintf(stderr, "[OSK] layer '%s' -> slot %d\n", name, slot);
            continue;
        }

        if (!cur) continue;

        if (*p == '\0') {
            if (cur_row < OSK_ROWS - 1) cur_row++;
            continue;
        }

        if (cur_row >= OSK_ROWS - 1) continue;

        char label_buf[32] = {0};
        char send_raw[64] = {0};
        char width_buf[8] = {0};

        char *tok = p;
        char *sep;

        sep = strchr(tok, '|');
        if (!sep) continue;

        size_t tlen = (size_t) (sep - tok);
        while (tlen > 0 && (unsigned char) tok[tlen - 1] <= ' ') tlen--;

        if (tlen == 0 || tlen >= sizeof(label_buf)) continue;

        memcpy(label_buf, tok, tlen);
        tok = sep + 1;

        while (*tok && (unsigned char) *tok <= ' ') tok++;

        sep = strchr(tok, '|');
        if (sep) {
            tlen = (size_t) (sep - tok);

            while (tlen > 0 && (unsigned char) tok[tlen - 1] <= ' ') tlen--;
            if (tlen >= sizeof(send_raw)) tlen = sizeof(send_raw) - 1;

            memcpy(send_raw, tok, tlen);
            tok = sep + 1;

            while (*tok && (unsigned char) *tok <= ' ') tok++;

            tlen = strlen(tok);
            while (tlen > 0 && (unsigned char) tok[tlen - 1] <= ' ') tlen--;

            if (tlen > 0 && tlen < sizeof(width_buf)) memcpy(width_buf, tok, tlen);
        } else {
            tlen = strlen(tok);
            while (tlen > 0 && (unsigned char) tok[tlen - 1] <= ' ') tlen--;

            if (tlen >= sizeof(send_raw)) tlen = sizeof(send_raw) - 1;
            memcpy(send_raw, tok, tlen);
        }

        int width = width_buf[0] ? atoi(width_buf) : 1;
        if (width < 1) width = 1;

        char send_decoded[64];
        parse_send(send_raw, send_decoded);

        char *hlabel = strdup(label_buf);
        char *hsend = strdup(send_decoded);

        if (!hlabel || !hsend) {
            free(hlabel);
            free(hsend);
            continue;
        }

        int col = 0;
        for (col = 0; col < OSK_MAX_COLS; col++) {
            if (!cur->grid[cur_row][col].label) break;
        }

        if (col < OSK_MAX_COLS) {
            cur->grid[cur_row][col].label = hlabel;
            cur->grid[cur_row][col].send = hsend;
            cur->grid[cur_row][col].width = width;
        } else {
            free(hlabel);
            free(hsend);
        }

    }

    for (int li = OSK_BUILTIN_LAYERS; li < osk_layer_count; li++) {
        OskKey (*grid)[OSK_MAX_COLS] = osk_layers[li];
        int c = 0;

        for (int ni = 0; ni < (int) (sizeof(row_nav) / sizeof(OskKey)) && c < OSK_MAX_COLS; ni++) {
            grid[OSK_ROWS - 1][c++] = row_nav[ni];
        }

        while (c < OSK_MAX_COLS) grid[OSK_ROWS - 1][c++] = OSK_EMPTY;
    }

    fclose(f);
    fprintf(stderr, "[OSK] loaded %d extra layer(s)\n", loaded);

    return loaded;
}

void osk_set_repeat(int delay_ms, int rate_ms) {
    if (delay_ms > 0) g_key_repeat_delay = (Uint32) delay_ms;
    if (rate_ms > 0) g_key_repeat_rate = (Uint32) rate_ms;
}

void osk_free(void) {
    for (int li = 0; li < g_dyn_layer_count; li++) {
        DynLayer *dl = g_dyn_layers[li];
        if (!dl) continue;

        for (int r = 0; r < OSK_ROWS - 1; r++) {
            for (int c = 0; c < OSK_MAX_COLS; c++) {
                if (dl->grid[r][c].label) {
                    free((void *) dl->grid[r][c].label);
                    dl->grid[r][c].label = NULL;
                }
                if (dl->grid[r][c].send) {
                    free((void *) dl->grid[r][c].send);
                    dl->grid[r][c].send = NULL;
                }
            }
        }

        free(dl);
        g_dyn_layers[li] = NULL;
    }

    g_dyn_layer_count = 0;
}

int osk_get_height(void) {
    return osk_height;
}

int osk_is_visible(void) {
    return osk_visible;
}

int osk_get_state(void) {
    return osk_state;
}

int osk_ctrl_active(void) {
    return osk_ctrl;
}

int osk_alt_active(void) {
    return osk_alt;
}

void osk_cycle_state(void) {
    osk_state = (osk_state + 1) % OSK_NUM_STATES;
    osk_visible = (osk_state != OSK_STATE_HIDDEN);

    fprintf(stderr, "[OSK] STATE -> %d visible=%d\n", osk_state, osk_visible);
}

void osk_reset_axis_state(void) {
    axis_x_state = 0;
    axis_y_state = 0;
}

void osk_move(int drow, int dcol) {
    int orig = osk_sel_row;
    osk_sel_row += drow;

    for (int t = 0; t < OSK_ROWS; t++) {
        if (osk_sel_row < 0) osk_sel_row = OSK_ROWS - 1;
        if (osk_sel_row >= OSK_ROWS) osk_sel_row = 0;
        if (row_len(osk_layer, osk_sel_row) > 0) break;
        osk_sel_row += (drow != 0) ? drow : 1;
    }

    int rlen = row_len(osk_layer, osk_sel_row);
    if (rlen == 0) {
        osk_sel_row = orig;
        rlen = row_len(osk_layer, osk_sel_row);
    }

    osk_sel_col += dcol;
    if (osk_sel_col < 0) osk_sel_col = rlen - 1;
    if (osk_sel_col >= rlen) osk_sel_col = 0;
}

void osk_switch_layer(int delta) {
    osk_layer = (osk_layer + delta + osk_layer_count) % osk_layer_count;
    int rlen = row_len(osk_layer, osk_sel_row);

    if (rlen == 0) {
        for (int r = 0; r < OSK_ROWS; r++) {
            if (row_len(osk_layer, r) > 0) {
                osk_sel_row = r;
                rlen = row_len(osk_layer, r);
                break;
            }
        }
    }

    if (osk_sel_col >= rlen) osk_sel_col = (rlen > 0) ? rlen - 1 : 0;
    if (osk_sel_col < 0) osk_sel_col = 0;
}

void osk_press_key(void) {
    const OskKey *key = &osk_layers[osk_layer][osk_sel_row][osk_sel_col];
    if (!key->label) return;

    if (strcmp(key->label, "Ctrl") == 0) {
        osk_ctrl = !osk_ctrl;
        return;
    }

    if (strcmp(key->label, "Alt") == 0) {
        osk_alt = !osk_alt;
        return;
    }

    const char *seq = key->send;
    char dyn_seq[8];

    if (strcmp(key->label, "Up") == 0 || strcmp(key->label, "Down") == 0 || strcmp(key->label, "Left") == 0 || strcmp(key->label, "Right") == 0) {
        char code;
        switch (key->label[0]) {
            case 'U':
                code = 'A';
                break;
            case 'D':
                code = 'B';
                break;
            case 'R':
                code = 'C';
                break;
            default:
                code = 'D';
                break;
        }
        snprintf(dyn_seq, sizeof(dyn_seq), vt_cursor_keys_app() ? "\x1BO%c" : "\x1B[%c", code);
        seq = dyn_seq;
    }

    int str_len = (int) strlen(seq);
    if (str_len <= 0) return;

    if (osk_alt) pty_write(g_pty_fd, "\x1B", 1);

    if (osk_ctrl && str_len == 1) {
        char ch = seq[0];
        if (ch >= 'a' && ch <= 'z') {
            ch = (char) (ch - 'a' + 1);
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = (char) (ch - 'A' + 1);
        }

        pty_write(g_pty_fd, &ch, 1);
    } else {
        pty_write(g_pty_fd, seq, str_len);
    }

    osk_ctrl = 0;
    osk_alt = 0;
}

void osk_hold_start(input_action_t action) {
    if (osk_hold_active) return;

    osk_hold_active = 1;

    g_hold_action = action;

    osk_hold_press_t = SDL_GetTicks();
    osk_hold_last_rep = osk_hold_press_t;
}

void osk_hold_end(void) {
    osk_hold_active = 0;
    g_hold_action = INPUT_ACT_NONE;
}

int osk_hold_tick(Uint32 now) {
    if (!osk_hold_active) return 0;

    if (now - osk_hold_press_t >= g_key_repeat_delay && now - osk_hold_last_rep >= g_key_repeat_rate) {
        osk_hold_last_rep = now;
        return 1;
    }

    return 0;
}

input_action_t osk_hold_action(void) {
    return g_hold_action;
}

void osk_handle_axis(int axis, int val) {
    const int dead = 16000;
    if (!osk_visible) return;

    if (axis == 0) {
        if (val < -dead && axis_x_state != -1) {
            axis_x_state = -1;
            osk_move(0, -1);
        } else if (val > dead && axis_x_state != 1) {
            axis_x_state = 1;
            osk_move(0, 1);
        } else if (val >= -dead && val <= dead) {
            axis_x_state = 0;
        }
    } else if (axis == 1) {
        if (val < -dead && axis_y_state != -1) {
            axis_y_state = -1;
            osk_move(-1, 0);
        } else if (val > dead && axis_y_state != 1) {
            axis_y_state = 1;
            osk_move(1, 0);
        } else if (val >= -dead && val <= dead) {
            axis_y_state = 0;
        }
    }
}

void osk_handle_hat(Uint8 hat) {
    if (!osk_visible) return;

    if (hat & SDL_HAT_UP) {
        osk_move(-1, 0);
    } else if (hat & SDL_HAT_DOWN) {
        osk_move(1, 0);
    } else if (hat & SDL_HAT_LEFT) {
        osk_move(0, -1);
    } else if (hat & SDL_HAT_RIGHT) {
        osk_move(0, 1);
    }
}

void osk_apply_action(input_action_t action, int *running, int *vis_rows, int term_height, int readonly, int pty_fd) {
    g_pty_fd = pty_fd;

    switch (action) {
        case INPUT_ACT_NONE:
            break;
        case INPUT_ACT_OSK_TOGGLE:
            osk_cycle_state();
            *vis_rows = (osk_state == OSK_STATE_BOTTOM_OPAQUE) ? (term_height - osk_height) / g_cell_h : vt_rows();
            if (*vis_rows < 1) *vis_rows = 1;

            vt_scroll_set(0);
            osk_reset_axis_state();
            break;
        case INPUT_ACT_QUIT:
            fprintf(stderr, "[OSK] QUIT\n");
            *running = 0;
            break;
        case INPUT_ACT_MENU:
            // Handled in main event loop!
            break;
        case INPUT_ACT_PRESS:
            if (!osk_visible) break;
            fprintf(stderr, "[OSK] PRESS KEY\n");
            osk_press_key();
            osk_hold_start(INPUT_ACT_PRESS);
            vt_scroll_set(0);
            break;
        case INPUT_ACT_BACKSPACE:
            fprintf(stderr, "[OSK] BACKSPACE\n");
            if (!readonly) {
                char bs = '\x7F';
                pty_write(pty_fd, &bs, 1);
            }
            osk_hold_start(INPUT_ACT_BACKSPACE);
            vt_scroll_set(0);
            break;
        case INPUT_ACT_SPACE:
            fprintf(stderr, "[OSK] SPACE\n");
            if (!readonly) {
                char sp = ' ';
                pty_write(pty_fd, &sp, 1);
            }
            osk_hold_start(INPUT_ACT_SPACE);
            vt_scroll_set(0);
            break;
        case INPUT_ACT_LAYER_PREV:
            if (!osk_visible) break;
            fprintf(stderr, "[OSK] LAYER -1\n");
            osk_switch_layer(-1);
            break;
        case INPUT_ACT_LAYER_NEXT:
            if (!osk_visible) break;
            fprintf(stderr, "[OSK] LAYER +1\n");
            osk_switch_layer(1);
            break;
        case INPUT_ACT_ENTER:
            fprintf(stderr, "[OSK] ENTER\n");
            if (!readonly) pty_write(pty_fd, "\r", 1);
            vt_scroll_set(0);
            break;
        case INPUT_ACT_PAGE_UP:
            fprintf(stderr, "[OSK] PAGE UP\n");
            vt_scroll_adjust(*vis_rows / 2, *vis_rows);
            break;
        case INPUT_ACT_PAGE_DOWN:
            fprintf(stderr, "[OSK] PAGE DOWN\n");
            vt_scroll_adjust(-(*vis_rows / 2), *vis_rows);
            break;
    }
}

void osk_render(SDL_Renderer *ren, TTF_Font *font, int screen_w, int screen_h) {
    if (!osk_visible) return;
    if (osk_height <= 0) {
        fprintf(stderr, "[OSK] ERROR: metrics not initialised\n");
        return;
    }

    int osk_at_top = (osk_state == OSK_STATE_TOP_OPAQUE || osk_state == OSK_STATE_TOP_TRANS);
    int osk_transparent = (osk_state == OSK_STATE_BOTTOM_TRANS || osk_state == OSK_STATE_TOP_TRANS);

    Uint8 backdrop_alpha = osk_transparent ? 115 : 230;
    Uint8 widget_alpha = osk_transparent ? 128 : 255;

    int draw_height = osk_height;
    if (draw_height > screen_h) draw_height = screen_h;

    int osk_y0 = osk_at_top ? 0 : (screen_h - draw_height);

    SDL_Color accent = {255, 200, 0, 255};
    SDL_Color key_dark = {40, 40, 40, 255};
    SDL_Color key_ctrl = {200, 100, 60, 255};
    SDL_Color key_sel = {255, 200, 0, 255};
    SDL_Color lbl_norm = {255, 200, 0, 255};
    SDL_Color lbl_sel = {0, 0, 0, 255};

    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
    SDL_Rect backdrop = {0, osk_y0, screen_w, draw_height};
    SDL_SetRenderDrawColor(ren, 18, 18, 18, backdrop_alpha);
    SDL_RenderFillRect(ren, &backdrop);

    {
        char info[64];
        char mod_buf[32] = {0};

        if (osk_ctrl) strcat(mod_buf, " [CTRL]");
        if (osk_alt) strcat(mod_buf, " [ALT]");

        snprintf(info, sizeof(info), " Layer: %s  %s", osk_layer_names[osk_layer], mod_buf);
        SDL_Surface *s = TTF_RenderUTF8_Blended(font, info, accent);

        if (s) {
            SDL_Texture *t = SDL_CreateTextureFromSurface(ren, s);
            if (t) {
                SDL_SetTextureAlphaMod(t, widget_alpha);
                SDL_Rect dst = {OSK_MARGIN, osk_y0 + 2, s->w, s->h};
                SDL_RenderCopy(ren, t, NULL, &dst);
                SDL_DestroyTexture(t);
            }
            SDL_FreeSurface(s);
        }
    }

    int keys_y0 = osk_y0 + osk_key_h + OSK_MARGIN;
    int keys_budget = (osk_y0 + draw_height) - keys_y0;
    int row_stride = osk_key_h + OSK_KEY_PAD;
    int visible_rows = (keys_budget > 0 && row_stride > 0) ? (keys_budget + OSK_KEY_PAD) / row_stride : 0;
    if (visible_rows > OSK_ROWS) visible_rows = OSK_ROWS;

    for (int row = 0; row < visible_rows; row++) {
        int rlen = row_len(osk_layer, row);
        if (rlen == 0) continue;

        int total_units = 0;
        for (int c = 0; c < rlen; c++) total_units += osk_layers[osk_layer][row][c].width;
        int avail_w = screen_w - 2 * OSK_MARGIN - (rlen - 1) * OSK_KEY_PAD;
        int unit_px = (total_units > 0) ? avail_w / total_units : osk_key_w;

        int total_w = 0;
        for (int c = 0; c < rlen; c++) total_w += osk_layers[osk_layer][row][c].width * unit_px;

        total_w += (rlen - 1) * OSK_KEY_PAD;

        int x = (screen_w - total_w) / 2;
        int y = keys_y0 + row * row_stride;

        if (y + osk_key_h > osk_y0 + draw_height) break;

        for (int col = 0; col < rlen; col++) {
            const OskKey *key = &osk_layers[osk_layer][row][col];

            int kw = key->width * unit_px;
            int sel = (row == osk_sel_row && col == osk_sel_col);

            int is_ctrl_key = strcmp(key->label, "Ctrl") == 0;
            int is_alt_key = strcmp(key->label, "Alt") == 0;

            SDL_Color fill = sel ? key_sel : ((is_ctrl_key && osk_ctrl) || (is_alt_key && osk_alt)) ? key_ctrl : key_dark;

            SDL_Rect kr = {x, y, kw, osk_key_h};
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, fill.r, fill.g, fill.b, widget_alpha);
            SDL_RenderFillRect(ren, &kr);
            SDL_SetRenderDrawColor(ren, 90, 90, 110, widget_alpha);
            SDL_RenderDrawRect(ren, &kr);

            SDL_Color lc = sel ? lbl_sel : lbl_norm;
            SDL_Surface *ls = TTF_RenderUTF8_Blended(font, key->label, lc);

            if (ls) {
                SDL_Texture *lt = SDL_CreateTextureFromSurface(ren, ls);
                if (lt) {
                    SDL_SetTextureAlphaMod(lt, widget_alpha);
                    SDL_Rect ld = {x + (kw - ls->w) / 2, y + (osk_key_h - ls->h) / 2, ls->w, ls->h};
                    SDL_RenderCopy(ren, lt, NULL, &ld);
                    SDL_DestroyTexture(lt);
                }
                SDL_FreeSurface(ls);
            }

            x += kw + OSK_KEY_PAD;
        }
    }
}
