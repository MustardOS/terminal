#include "../lang.h"

const LangDef lang_pl = {

        /* Metadata */
        .lang_code    = "pl",
        .lang_name    = "Polski",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_terminal_font_size = "Rozmiar Czcionki Terminala",
        .item_menu_font_size     = "Rozmiar Czcionki Menu",
        .item_font_hinting       = "Wygładzanie Czcionki",
        .item_fg_colour          = "Kolor Tekstu",
        .item_bg_colour          = "Kolor Tła",
        .item_language           = "Język",
        .item_save_config        = "Zapisz Konfigurację",
        .item_reset_config       = "Resetuj Konfigurację",
        .item_quit               = "Wyjdź z muTerm",

        /* Font hinting mode names */
        .hint_normal = "normalny",
        .hint_light  = "lekki",
        .hint_mono   = "mono",
        .hint_none   = "brak",

        /* Foreground colour preset names */
        .fg_white      = "Biały",
        .fg_amber      = "Bursztynowy",
        .fg_yellow     = "Żółty",
        .fg_green      = "Zielony",
        .fg_lime       = "Limonkowy",
        .fg_cyan       = "Cyjan",
        .fg_light_blue = "Jasnoniebieski",
        .fg_blue       = "Niebieski",
        .fg_pink       = "Różowy",
        .fg_magenta    = "Magenta",
        .fg_orange     = "Pomarańczowy",
        .fg_light_gray = "Jasnoszary",

        /* Background colour preset names */
        .bg_black       = "Czarny",
        .bg_dark_gray   = "Ciemnoszary",
        .bg_charcoal    = "Antracyt",
        .bg_dark_blue   = "Ciemnoniebieski",
        .bg_navy        = "Granatowy",
        .bg_dark_green  = "Ciemnozielony",
        .bg_forest      = "Leśny",
        .bg_dark_purple = "Ciemnofioletowy",
        .bg_plum        = "Śliwkowy",
        .bg_dark_red    = "Ciemnoczerwony",
        .bg_dark_teal   = "Ciemny Turkus",
        .bg_dark_brown  = "Ciemnobrązowy",

        /* Menu navigation hint */
        .menu_hint = "  G/D: nawigacja   L/P: zmiana   A/Start: akcja   B/Guide: zamknij  ",

        /* Toast notifications */
        .notify_config_saved = "  Konfiguracja zapisana  ",
        .notify_config_reset = "  Konfiguracja zresetowana  ",
};
