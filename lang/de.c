#include "../lang.h"

const LangDef lang_de = {

        /* Metadata */
        .lang_code    = "de",
        .lang_name    = "Deutsch",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_terminal_font_size = "Terminal-Schriftgröße",
        .item_menu_font_size     = "Menü-Schriftgröße",
        .item_font_hinting       = "Schrift-Hinting",
        .item_fg_colour          = "Vordergrundfarbe",
        .item_bg_colour          = "Hintergrundfarbe",
        .item_language           = "Sprache",
        .item_save_config        = "Konfiguration Speichern",
        .item_reset_config       = "Konfiguration Zurücksetzen",
        .item_quit               = "muTerm Beenden",

        /* Font hinting mode names */
        .hint_normal = "normal",
        .hint_light  = "leicht",
        .hint_mono   = "mono",
        .hint_none   = "keins",

        /* Foreground colour preset names */
        .fg_white      = "Weiß",
        .fg_amber      = "Bernstein",
        .fg_yellow     = "Gelb",
        .fg_green      = "Grün",
        .fg_lime       = "Limette",
        .fg_cyan       = "Cyan",
        .fg_light_blue = "Hellblau",
        .fg_blue       = "Blau",
        .fg_pink       = "Pink",
        .fg_magenta    = "Magenta",
        .fg_orange     = "Orange",
        .fg_light_gray = "Hellgrau",

        /* Background colour preset names */
        .bg_black       = "Schwarz",
        .bg_dark_gray   = "Dunkelgrau",
        .bg_charcoal    = "Anthrazit",
        .bg_dark_blue   = "Dunkelblau",
        .bg_navy        = "Marine",
        .bg_dark_green  = "Dunkelgrün",
        .bg_forest      = "Waldgrün",
        .bg_dark_purple = "Dunkellila",
        .bg_plum        = "Pflaume",
        .bg_dark_red    = "Dunkelrot",
        .bg_dark_teal   = "Dunkles Blaugrün",
        .bg_dark_brown  = "Dunkelbraun",

        /* Menu navigation hint */
        .menu_hint = "  O/U: navigieren   L/R: anpassen   A/Start: Aktion   B/Guide: schließen  ",

        /* Toast notifications */
        .notify_config_saved = "  Konfiguration gespeichert  ",
        .notify_config_reset = "  Konfiguration zurückgesetzt  ",
};
