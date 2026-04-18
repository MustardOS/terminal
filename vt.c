#include <stdlib.h>
#include <string.h>
#include "vt.h"

static Cell *screen_buf = NULL;
static Cell *main_screen_buf = NULL;
static Cell *alt_screen_buf = NULL;

static int using_alt_screen = 0;

static int saved_main_row = 0;
static int saved_main_col = 0;

static int vt_g0_charset = 0;
static int vt_g1_charset = 0;
static int vt_gl_charset = 0;

static unsigned char esc_pending[128];
static size_t esc_pending_len = 0;

static int cursor_keys_application = 0;
static int linefeed_mode = 0;

static int scroll_top = 0;
static int scroll_bottom = 0;

static int TERM_COLS = 0;
static int TERM_ROWS = 0;

static int cursor_row = 0;
static int cursor_col = 0;
static int cursor_vis = 1;

static int saved_row = 0;
static int saved_col = 0;

static SDL_Color default_fg = {255, 255, 255, 255};
static SDL_Color default_bg = {0, 0, 0, 255};
static SDL_Color current_fg;
static SDL_Color current_bg;
static Uint8 current_style = 0;

static int screen_dirty = 1;

static Cell *scrollback = NULL;
static int sb_capacity = 512;
static int sb_count = 0;
static int sb_head = 0;
static int scroll_offset = 0;

static const SDL_Color base_colours[8] = {
        {0,   0,   0,   255},
        {170, 0,   0,   255},
        {0,   170, 0,   255},
        {170, 85,  0,   255},
        {0,   0,   170, 255},
        {170, 0,   170, 255},
        {0,   170, 170, 255},
        {170, 170, 170, 255},
};

static const SDL_Color bright_colours[8] = {
        {85,  85,  85,  255},
        {255, 85,  85,  255},
        {85,  255, 85,  255},
        {255, 255, 85,  255},
        {85,  85,  255, 255},
        {255, 85,  255, 255},
        {85,  255, 255, 255},
        {255, 255, 255, 255},
};

static const Uint8 cube6[6] = {0, 95, 135, 175, 215, 255};

static inline Cell *CELL(int r, int c) {
    return &screen_buf[(size_t) r * (size_t) TERM_COLS + (size_t) c];
}

static inline int row_in_scroll_region(int row) {
    return row >= scroll_top && row <= scroll_bottom;
}

static Uint32 vt_map_acs(Uint32 cp) {
    switch (cp) {
        case '`':
            return 0x25C6;
        case 'a':
            return 0x2592;
        case 'b':
            return 0x2409;
        case 'c':
            return 0x240C;
        case 'd':
            return 0x240D;
        case 'e':
            return 0x240A;
        case 'f':
            return 0x00B0;
        case 'g':
            return 0x00B1;
        case 'h':
            return 0x2424;
        case 'i':
            return 0x240B;
        case 'j':
            return 0x2518;
        case 'k':
            return 0x2510;
        case 'l':
            return 0x250C;
        case 'm':
            return 0x2514;
        case 'n':
            return 0x253C;
        case 'o':
            return 0x23BA;
        case 'p':
            return 0x23BB;
        case 'q':
            return 0x2500;
        case 'r':
            return 0x23BC;
        case 's':
            return 0x23BD;
        case 't':
            return 0x251C;
        case 'u':
            return 0x2524;
        case 'v':
            return 0x2534;
        case 'w':
            return 0x252C;
        case 'x':
            return 0x2502;
        case 'y':
            return 0x2264;
        case 'z':
            return 0x2265;
        case '{':
            return 0x03C0;
        case '|':
            return 0x2260;
        case '}':
            return 0x00A3;
        case '~':
            return 0x00B7;
        default:
            return cp;
    }
}

static inline int vt_gl_is_graphics(void) {
    return vt_gl_charset ? (vt_g1_charset == 1) : (vt_g0_charset == 1);
}

static inline Uint32 vt_apply_graphics_charset(Uint32 cp) {
    if (cp < 0x20 || cp > 0x7E || !vt_gl_is_graphics()) return cp;
    return vt_map_acs(cp);
}

static void vt_designate_charset(int which, char final) {
    int mode = (final == '0') ? 1 : 0;
    if (which == 0) {
        vt_g0_charset = mode;
    } else {
        vt_g1_charset = mode;
    }
}

static void vt_shift_in(void) { vt_gl_charset = 0; }

static void vt_shift_out(void) { vt_gl_charset = 1; }

static inline void reset_cell(Cell *c) {
    c->codepoint = (Uint32) ' ';
    c->width = 1;
    c->fg = current_fg;
    c->bg = current_bg;
    c->style = 0;
}

static void clear_cells(Cell *buf) {
    size_t total = (size_t) TERM_ROWS * (size_t) TERM_COLS;

    for (size_t i = 0; i < total; i++) {
        buf[i].codepoint = (Uint32) ' ';
        buf[i].width = 1;
        buf[i].fg = current_fg;
        buf[i].bg = current_bg;
        buf[i].style = 0;
    }
}

static void clear_screen(void) {
    size_t total = (size_t) TERM_ROWS * (size_t) TERM_COLS;
    for (size_t i = 0; i < total; i++) reset_cell(&screen_buf[i]);
}

static void clear_row_range(int row, int start_col, int end_col) {
    if (row < 0 || row >= TERM_ROWS) return;
    if (start_col < 0) start_col = 0;
    if (end_col >= TERM_COLS) end_col = TERM_COLS - 1;
    if (start_col > end_col) return;

    for (int c = start_col; c <= end_col; c++) reset_cell(CELL(row, c));
}

static void set_cursor(int row, int col) {
    if (row < 0) row = 0;
    if (row >= TERM_ROWS) row = TERM_ROWS - 1;

    if (col < 0) col = 0;
    if (col >= TERM_COLS) col = TERM_COLS - 1;

    cursor_row = row;
    cursor_col = col;
}

static void scrollback_push(const Cell *row) {
    memcpy(&scrollback[(size_t) sb_head * (size_t) TERM_COLS], row, (size_t) TERM_COLS * sizeof(Cell));
    sb_head = (sb_head + 1) % sb_capacity;

    if (sb_count < sb_capacity) sb_count++;
}

static void scroll_region_up(int top, int bottom, int lines, int allow_scrollback) {
    if (TERM_ROWS <= 1) return;
    if (top < 0) top = 0;
    if (bottom >= TERM_ROWS) bottom = TERM_ROWS - 1;
    if (top >= bottom || lines <= 0) return;

    int height = bottom - top + 1;
    if (lines > height) lines = height;

    if (allow_scrollback && !using_alt_screen && top == 0 && bottom == TERM_ROWS - 1) {
        for (int i = 0; i < lines; i++) scrollback_push(CELL(top + i, 0));
    }

    if (lines < height) memmove(CELL(top, 0), CELL(top + lines, 0), sizeof(Cell) * (size_t) TERM_COLS * (size_t) (height - lines));

    for (int r = bottom - lines + 1; r <= bottom; r++) {
        clear_row_range(r, 0, TERM_COLS - 1);
    }
}

static void scroll_region_down(int top, int bottom, int lines) {
    if (TERM_ROWS <= 1) return;
    if (top < 0) top = 0;
    if (bottom >= TERM_ROWS) bottom = TERM_ROWS - 1;
    if (top >= bottom || lines <= 0) return;

    int height = bottom - top + 1;
    if (lines > height) lines = height;

    if (lines < height) memmove(CELL(top + lines, 0), CELL(top, 0), sizeof(Cell) * (size_t) TERM_COLS * (size_t) (height - lines));

    for (int r = top; r < top + lines; r++) {
        clear_row_range(r, 0, TERM_COLS - 1);
    }
}

static void vt_index(void) {
    if (row_in_scroll_region(cursor_row)) {
        if (cursor_row == scroll_bottom) {
            scroll_region_up(scroll_top, scroll_bottom, 1, 1);
        } else {
            cursor_row++;
        }
        return;
    }

    if (cursor_row < TERM_ROWS - 1) {
        cursor_row++;
    } else if (scroll_top == 0 && scroll_bottom == TERM_ROWS - 1) {
        scroll_region_up(0, TERM_ROWS - 1, 1, 1);
    }
}

static void vt_reverse_index(void) {
    if (row_in_scroll_region(cursor_row)) {
        if (cursor_row == scroll_top) {
            scroll_region_down(scroll_top, scroll_bottom, 1);
        } else {
            cursor_row--;
        }
        return;
    }

    if (cursor_row > 0) {
        cursor_row--;
    } else if (scroll_top == 0 && scroll_bottom == TERM_ROWS - 1) {
        scroll_region_down(0, TERM_ROWS - 1, 1);
    }
}

static void vt_newline(void) {
    if (linefeed_mode) cursor_col = 0;
    vt_index();
}

static void vt_reset_state(void) {
    cursor_keys_application = 0;
    linefeed_mode = 0;

    scroll_top = 0;
    scroll_bottom = TERM_ROWS - 1;

    cursor_vis = 1;

    saved_row = 0;
    saved_col = 0;

    saved_main_row = 0;
    saved_main_col = 0;

    current_fg = default_fg;
    current_bg = default_bg;

    current_style = 0;

    vt_g0_charset = 0;
    vt_g1_charset = 0;
    vt_gl_charset = 0;

    screen_buf = main_screen_buf;
    using_alt_screen = 0;

    if (main_screen_buf) clear_cells(main_screen_buf);
    if (alt_screen_buf) clear_cells(alt_screen_buf);

    set_cursor(0, 0);
    scroll_offset = 0;
    screen_dirty = 1;
}

static void vt_enter_alt_screen(void) {
    if (using_alt_screen) return;

    saved_main_row = cursor_row;
    saved_main_col = cursor_col;

    screen_buf = alt_screen_buf;
    using_alt_screen = 1;

    clear_cells(screen_buf);

    cursor_row = 0;
    cursor_col = 0;

    scroll_offset = 0;
    screen_dirty = 1;
}

static void vt_leave_alt_screen(void) {
    if (!using_alt_screen) return;
    screen_buf = main_screen_buf;

    using_alt_screen = 0;
    cursor_row = saved_main_row;
    cursor_col = saved_main_col;

    scroll_offset = 0;
    screen_dirty = 1;
}

static SDL_Color colour_from_256(int idx) {
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;

    if (idx < 8) return base_colours[idx];
    if (idx < 16) return bright_colours[idx - 8];

    if (idx < 232) {
        int v = idx - 16;
        int ri = v / 36;
        int gi = (v % 36) / 6;
        int bi = v % 6;
        return (SDL_Color) {cube6[ri], cube6[gi], cube6[bi], 255};
    }

    Uint8 g = (Uint8) (8 + 10 * (idx - 232));
    return (SDL_Color) {g, g, g, 255};
}

static void apply_sgr(int *params, int count) {
    if (count == 0) {
        current_fg = default_fg;
        current_bg = default_bg;
        current_style = 0;
        return;
    }

    for (int i = 0; i < count; i++) {
        int p = params[i];
        switch (p) {
            case 0:
                current_fg = default_fg;
                current_bg = default_bg;
                current_style = 0;
                break;
            case 1:
                current_style |= STYLE_BOLD;
                break;
            case 4:
                current_style |= STYLE_UNDERLINE;
                break;
            case 7:
                current_style |= STYLE_REVERSE;
                break;
            case 22:
                current_style &= (Uint8) ~STYLE_BOLD;
                break;
            case 24:
                current_style &= (Uint8) ~STYLE_UNDERLINE;
                break;
            case 27:
                current_style &= (Uint8) ~STYLE_REVERSE;
                break;
            case 39:
                current_fg = default_fg;
                break;
            case 49:
                current_bg = default_bg;
                break;
            default:
                if (p >= 30 && p <= 37) {
                    current_fg = (current_style & STYLE_BOLD) ? bright_colours[p - 30] : base_colours[p - 30];
                } else if (p >= 40 && p <= 47) {
                    current_bg = base_colours[p - 40];
                } else if (p >= 90 && p <= 97) {
                    current_fg = bright_colours[p - 90];
                } else if (p >= 100 && p <= 107) {
                    current_bg = bright_colours[p - 100];
                } else if (p == 38) {
                    if (i + 1 < count && params[i + 1] == 5 && i + 2 < count) {
                        current_fg = colour_from_256(params[i + 2]);
                        i += 2;
                    } else if (i + 1 < count && params[i + 1] == 2 && i + 4 < count) {
                        current_fg = (SDL_Color) {(Uint8) params[i + 2], (Uint8) params[i + 3], (Uint8) params[i + 4], 255};
                        i += 4;
                    }
                } else if (p == 48) {
                    if (i + 1 < count && params[i + 1] == 5 && i + 2 < count) {
                        current_bg = colour_from_256(params[i + 2]);
                        i += 2;
                    } else if (i + 1 < count && params[i + 1] == 2 && i + 4 < count) {
                        current_bg = (SDL_Color) {(Uint8) params[i + 2], (Uint8) params[i + 3], (Uint8) params[i + 4], 255};
                        i += 4;
                    }
                }
                break;
        }
    }
}

static inline int is_csi_final(unsigned char ch) {
    return ch >= 0x40 && ch <= 0x7E;
}

static inline int parse_int_fast(const char **p) {
    int v = 0;

    while (**p >= '0' && **p <= '9') {
        v = v * 10 + (**p - '0');
        (*p)++;
    }

    return v;
}

static void parse_csi(const char *seq) {
    if (seq[0] != '[') return;

    int params[16] = {0};
    int count = 0;

    int is_private = 0;
    const char *p = seq + 1;

    if (*p == '?') {
        is_private = 1;
        p++;
    }

    while (*p && !is_csi_final((unsigned char) *p)) {
        if (*p >= '0' && *p <= '9') {
            if (count < 16) params[count] = parse_int_fast(&p);
            else { while (*p >= '0' && *p <= '9') p++; }
        } else if (*p == ';') {
            if (count < 15) count++;
            p++;
        } else { p++; }
    }

    count++;

    char cmd = *p ? *p : '\0';
    int p0 = params[0];
    int p1 = (count > 1) ? params[1] : 0;

    switch (cmd) {
        case 'A':
            set_cursor(cursor_row - (p0 ? p0 : 1), cursor_col);
            break;
        case 'B':
            set_cursor(cursor_row + (p0 ? p0 : 1), cursor_col);
            break;
        case 'C':
            set_cursor(cursor_row, cursor_col + (p0 ? p0 : 1));
            break;
        case 'D':
            set_cursor(cursor_row, cursor_col - (p0 ? p0 : 1));
            break;
        case 'E':
            set_cursor(cursor_row + (p0 ? p0 : 1), 0);
            break;
        case 'F':
            set_cursor(cursor_row - (p0 ? p0 : 1), 0);
            break;
        case 'G':
            set_cursor(cursor_row, (p0 ? p0 : 1) - 1);
            break;
        case 'd':
            set_cursor((p0 ? p0 : 1) - 1, cursor_col);
            break;
        case 'H':
        case 'f':
            set_cursor((p0 ? p0 : 1) - 1, (p1 ? p1 : 1) - 1);
            break;
        case 'J': {
            if (p0 == 0) {
                clear_row_range(cursor_row, cursor_col, TERM_COLS - 1);
                for (int r = cursor_row + 1; r < TERM_ROWS; r++) clear_row_range(r, 0, TERM_COLS - 1);
            } else if (p0 == 1) {
                for (int r = 0; r < cursor_row; r++) clear_row_range(r, 0, TERM_COLS - 1);
                clear_row_range(cursor_row, 0, cursor_col);
            } else if (p0 == 2) {
                clear_screen();
                set_cursor(0, 0);
            }
            break;
        }
        case 'K': {
            if (p0 == 0) {
                clear_row_range(cursor_row, cursor_col, TERM_COLS - 1);
            } else if (p0 == 1) {
                clear_row_range(cursor_row, 0, cursor_col);
            } else if (p0 == 2) {
                clear_row_range(cursor_row, 0, TERM_COLS - 1);
            }
            break;
        }
        case 'L': {
            int n = p0 ? p0 : 1;
            if (row_in_scroll_region(cursor_row)) scroll_region_down(cursor_row, scroll_bottom, n);
            break;
        }
        case 'M': {
            int n = p0 ? p0 : 1;
            if (row_in_scroll_region(cursor_row)) scroll_region_up(cursor_row, scroll_bottom, n, 0);
            break;
        }
        case 'P': {
            int n = p0 ? p0 : 1;
            if (n < 0) n = 0;
            if (cursor_col + n > TERM_COLS) n = TERM_COLS - cursor_col;
            if (n > 0) {
                memmove(CELL(cursor_row, cursor_col), CELL(cursor_row, cursor_col + n), sizeof(Cell) * (size_t) (TERM_COLS - cursor_col - n));
                for (int c = TERM_COLS - n; c < TERM_COLS; c++) reset_cell(CELL(cursor_row, c));
            }
            break;
        }
        case '@': {
            int n = p0 ? p0 : 1;
            if (n < 0) n = 0;
            if (cursor_col + n > TERM_COLS) n = TERM_COLS - cursor_col;
            if (n > 0) {
                memmove(CELL(cursor_row, cursor_col + n), CELL(cursor_row, cursor_col), sizeof(Cell) * (size_t) (TERM_COLS - cursor_col - n));
                for (int c = cursor_col; c < cursor_col + n; c++) reset_cell(CELL(cursor_row, c));
            }
            break;
        }
        case 'S':
            scroll_region_up(scroll_top, scroll_bottom, p0 ? p0 : 1, 1);
            break;
        case 'T':
            scroll_region_down(scroll_top, scroll_bottom, p0 ? p0 : 1);
            break;
        case 'X': {
            int n = p0 ? p0 : 1;
            for (int c = cursor_col; c < cursor_col + n && c < TERM_COLS; c++) reset_cell(CELL(cursor_row, c));
            break;
        }
        case 'm':
            apply_sgr(params, count);
            break;
        case 'r': {
            int top = (p0 ? p0 : 1) - 1;
            int bottom = (p1 ? p1 : TERM_ROWS) - 1;
            if (top < 0) top = 0;
            if (bottom >= TERM_ROWS) bottom = TERM_ROWS - 1;
            if (top >= bottom) {
                scroll_top = 0;
                scroll_bottom = TERM_ROWS - 1;
            } else {
                scroll_top = top;
                scroll_bottom = bottom;
            }
            set_cursor(0, 0);
            break;
        }
        case 's':
            saved_row = cursor_row;
            saved_col = cursor_col;
            break;
        case 'u':
            set_cursor(saved_row, saved_col);
            break;
        case 'h':
        case 'l': {
            int enable = (cmd == 'h');
            for (int i = 0; i < count; i++) {
                int mode = params[i];
                if (is_private) {
                    switch (mode) {
                        case 1:
                            cursor_keys_application = enable;
                            break;
                        case 25:
                            cursor_vis = enable;
                            break;
                        case 47:
                        case 1047:
                        case 1049:
                            if (enable) vt_enter_alt_screen();
                            else vt_leave_alt_screen();
                            break;
                        default:
                            break;
                    }
                } else {
                    if (mode == 20) linefeed_mode = enable;
                }
            }
            break;
        }
        default:
            break;
    }

    screen_dirty = 1;
}

static void handle_esc(const char *seq) {
    if (!*seq) return;
    switch (seq[0]) {
        case '(':
            if (seq[1]) vt_designate_charset(0, seq[1]);
            break;
        case ')':
            if (seq[1]) vt_designate_charset(1, seq[1]);
            break;
        case '7':
            saved_row = cursor_row;
            saved_col = cursor_col;
            break;
        case '8':
            set_cursor(saved_row, saved_col);
            break;
        case 'D':
            vt_index();
            break;
        case 'E':
            cursor_col = 0;
            vt_index();
            break;
        case 'M':
            vt_reverse_index();
            break;
        case '=':
        case '>':
            break;
        case 'c':
            vt_reset_state();
            break;
        default:
            break;
    }
}

static size_t vt_try_parse_escape(const unsigned char *buf, size_t len) {
    if (len == 0 || buf[0] != 0x1B || len == 1) return 0;

    if (buf[1] == '[') {
        size_t i = 2;
        while (i < len && !is_csi_final(buf[i])) i++;
        if (i >= len) return 0;

        char tmp[64];
        size_t slen = i < sizeof(tmp) ? i : sizeof(tmp) - 1;
        memcpy(tmp, buf + 1, slen);
        tmp[slen] = '\0';
        parse_csi(tmp);

        return i + 1;
    }

    if (buf[1] == '(' || buf[1] == ')') {
        if (len < 3) return 0;

        char tmp[4] = {(char) buf[1], (char) buf[2], '\0', '\0'};
        handle_esc(tmp);

        return 3;
    }

    char tmp[2] = {(char) buf[1], '\0'};
    handle_esc(tmp);

    return 2;
}

static int wcwidth_emu(Uint32 cp) {
    if (cp == 0 || cp < 32 || (cp >= 0x7F && cp < 0xA0) || (cp >= 0x0300 && cp <= 0x036F)) return 0;

    if ((cp >= 0x1100 && cp <= 0x115F) ||
        (cp >= 0x2329 && cp <= 0x232A) ||
        (cp >= 0x2E80 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6))
        return 2;

    return 1;
}

static size_t utf8_decode_char(Uint32 *out, const unsigned char *s, size_t len) {
    if (!s || len == 0) return 0;
    unsigned char b0 = s[0];

    if (b0 < 0x80) {
        *out = b0;
        return 1;
    }

    if ((b0 & 0xE0) == 0xC0) {
        if (len < 2) return (size_t) -2;

        unsigned char b1 = s[1];
        if ((b1 & 0xC0) != 0x80) return (size_t) -1;

        Uint32 cp = ((Uint32) (b0 & 0x1F) << 6) | (Uint32) (b1 & 0x3F);
        if (cp < 0x80) return (size_t) -1;

        *out = cp;

        return 2;
    }

    if ((b0 & 0xF0) == 0xE0) {
        if (len < 3) return (size_t) -2;

        unsigned char b1 = s[1], b2 = s[2];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80) return (size_t) -1;

        Uint32 cp = ((Uint32) (b0 & 0x0F) << 12) | ((Uint32) (b1 & 0x3F) << 6) | (Uint32) (b2 & 0x3F);
        if (cp < 0x800 || (cp >= 0xD800 && cp <= 0xDFFF)) return (size_t) -1;

        *out = cp;

        return 3;
    }

    if ((b0 & 0xF8) == 0xF0) {
        if (len < 4) return (size_t) -2;

        unsigned char b1 = s[1], b2 = s[2], b3 = s[3];
        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) return (size_t) -1;

        Uint32 cp = ((Uint32) (b0 & 0x07) << 18) | ((Uint32) (b1 & 0x3F) << 12) | ((Uint32) (b2 & 0x3F) << 6) | (Uint32) (b3 & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) return (size_t) -1;

        *out = cp;

        return 4;
    }

    return (size_t) -1;
}

static void put_char(Uint32 ch) {
    if (ch == '\n' || ch == '\v' || ch == '\f') {
        vt_newline();
        return;
    }

    if (ch == '\r') {
        cursor_col = 0;
        screen_dirty = 1;
        return;
    }

    if (ch == '\b') {
        if (cursor_col > 0) cursor_col--;
        return;
    }

    if (ch == '\t') {
        int next = ((cursor_col / 8) + 1) * 8;
        cursor_col = (next >= TERM_COLS) ? TERM_COLS - 1 : next;
        return;
    }

    ch = vt_apply_graphics_charset(ch);

    int w = wcwidth_emu(ch);
    if (w <= 0) return;

    if (cursor_col >= TERM_COLS || cursor_col + w > TERM_COLS) {
        cursor_col = 0;
        vt_index();
    }

    if (cursor_row >= TERM_ROWS) cursor_row = TERM_ROWS - 1;

    Cell *c = CELL(cursor_row, cursor_col);

    c->codepoint = ch;
    c->width = (Uint8) w;
    c->fg = current_fg;
    c->bg = current_bg;
    c->style = current_style;

    if (w == 2 && cursor_col + 1 < TERM_COLS) {
        Cell *c2 = CELL(cursor_row, cursor_col + 1);
        c2->codepoint = 0;
        c2->width = 0;
        c2->fg = c->fg;
        c2->bg = c->bg;
        c2->style = c->style;
    }

    cursor_col += w;
}

static int vt_process_stream_bytes(const unsigned char *buf, size_t len) {
    size_t pos = 0;
    int saw_output = 0;

    while (pos < len) {
        unsigned char ch = buf[pos];

        if (ch == 0x0E) {
            vt_shift_out();
            screen_dirty = 1;
            pos++;
            continue;
        }

        if (ch == 0x0F) {
            vt_shift_in();
            screen_dirty = 1;
            pos++;
            continue;
        }

        if (ch == 0x1B) {
            size_t consumed = vt_try_parse_escape(buf + pos, len - pos);
            if (consumed == 0) break;
            pos += consumed;
            saw_output = 1;
            continue;
        }

        Uint32 cp = 0;
        size_t consumed = utf8_decode_char(&cp, buf + pos, len - pos);

        if (consumed == (size_t) -2) break;

        if (consumed == (size_t) -1) {
            put_char(0xFFFD);
            screen_dirty = 1;
            saw_output = 1;
            pos++;
            continue;
        }

        put_char(cp);
        screen_dirty = 1;

        saw_output = 1;
        pos += consumed;
    }

    if (pos < len) {
        size_t rem = len - pos;
        if (rem > sizeof(esc_pending)) rem = sizeof(esc_pending);

        memcpy(esc_pending, buf + pos, rem);
        esc_pending_len = rem;
    } else {
        esc_pending_len = 0;
    }

    if (saw_output) screen_dirty = 1;

    return saw_output;
}

int vt_init(int cols, int rows, int scrollback_capacity) {
    TERM_COLS = (cols > 0) ? cols : 80;
    TERM_ROWS = (rows > 0) ? rows : 24;

    sb_capacity = (scrollback_capacity > 0) ? scrollback_capacity : 512;

    main_screen_buf = calloc((size_t) TERM_ROWS * (size_t) TERM_COLS, sizeof(Cell));
    alt_screen_buf = calloc((size_t) TERM_ROWS * (size_t) TERM_COLS, sizeof(Cell));
    scrollback = calloc((size_t) sb_capacity * (size_t) TERM_COLS, sizeof(Cell));

    if (!main_screen_buf || !alt_screen_buf || !scrollback) {
        free(main_screen_buf);
        main_screen_buf = NULL;

        free(alt_screen_buf);
        alt_screen_buf = NULL;

        free(scrollback);
        scrollback = NULL;

        return -1;
    }

    sb_count = 0;
    sb_head = 0;

    screen_buf = main_screen_buf;
    using_alt_screen = 0;

    current_fg = default_fg;
    current_bg = default_bg;

    current_style = 0;

    vt_reset_state();

    return 0;
}

void vt_free(void) {
    free(main_screen_buf);
    main_screen_buf = NULL;

    free(alt_screen_buf);
    alt_screen_buf = NULL;

    free(scrollback);
    scrollback = NULL;

    screen_buf = NULL;
}

int vt_cols(void) {
    return TERM_COLS;
}

int vt_rows(void) {
    return TERM_ROWS;
}

int vt_cursor_row(void) {
    return cursor_row;
}

int vt_cursor_col(void) {
    return cursor_col;
}

int vt_cursor_visible(void) {
    return cursor_vis;
}

int vt_cursor_keys_app(void) {
    return cursor_keys_application;
}

int vt_scrollback_count(void) {
    return sb_count;
}

int vt_using_alt_screen(void) {
    return using_alt_screen;
}

int vt_is_dirty(void) {
    return screen_dirty;
}

void vt_clear_dirty(void) {
    screen_dirty = 0;
}

int vt_scroll_offset(void) {
    return scroll_offset;
}

void vt_scroll_set(int offset) {
    if (offset < 0) offset = 0;
    if (offset > sb_count) offset = sb_count;

    scroll_offset = offset;
    screen_dirty = 1;
}

void vt_scroll_adjust(int delta, int max_visible_rows) {
    (void) max_visible_rows;
    int new_off = scroll_offset + delta;

    if (new_off > sb_count) new_off = sb_count;
    if (new_off < 0) new_off = 0;

    scroll_offset = new_off;
    screen_dirty = 1;
}

const Cell *vt_scrollback_row(int idx) {
    if (idx < 0 || idx >= sb_count) return NULL;

    int pos = (sb_head - sb_count + idx + sb_capacity) % sb_capacity;
    return &scrollback[(size_t) pos * (size_t) TERM_COLS];
}

Cell *vt_cell(int row, int col) {
    if (row < 0) row = 0;
    if (row >= TERM_ROWS) row = TERM_ROWS - 1;

    if (col < 0) col = 0;
    if (col >= TERM_COLS) col = TERM_COLS - 1;

    return CELL(row, col);
}

int vt_feed(const char *buf, size_t len) {
    unsigned char merged[sizeof(esc_pending) + 4096];
    size_t total = 0;

    if (esc_pending_len > 0) {
        memcpy(merged, esc_pending, esc_pending_len);
        total = esc_pending_len;
    }

    if (len > sizeof(merged) - total) len = sizeof(merged) - total;
    memcpy(merged + total, buf, len);

    total += len;

    esc_pending_len = 0;
    return vt_process_stream_bytes(merged, total);
}
