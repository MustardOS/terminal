#include <stddef.h>
#include "lang.h"

extern const LangDef lang_en;
extern const LangDef lang_es;
extern const LangDef lang_pl;
extern const LangDef lang_zh;
extern const LangDef lang_de;
extern const LangDef lang_vi;
extern const LangDef lang_el;
extern const LangDef lang_au;

const LangDef *const g_lang_table[] = {
        &lang_en,
        &lang_es,
        &lang_pl,
        &lang_zh,
        &lang_de,
        &lang_vi,
        &lang_el,
        &lang_au,
        NULL,
};

const int g_lang_count = 8;

const LangDef *g_lang = NULL;

static int g_lang_index = 0;

void lang_init(void) {
    g_lang_index = 0;
    g_lang = g_lang_table[0];
}

void lang_set(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= g_lang_count) idx = g_lang_count - 1;

    g_lang_index = idx;
    g_lang = g_lang_table[idx];
}

int lang_current_index(void) {
    return g_lang_index;
}
