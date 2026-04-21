#include "../lang.h"

const LangDef lang_en = {

        /* Metadata */
        .lang_code    = "en",
        .lang_name    = "English",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_terminal_font_size = "Terminal Font Size",
        .item_menu_font_size     = "Menu Font Size",
        .item_font_hinting       = "Font Hinting",
        .item_fg_colour          = "Foreground Colour",
        .item_bg_colour          = "Background Colour",
        .item_language           = "Language",
        .item_save_config        = "Save Config",
        .item_reset_config       = "Reset Config",
        .item_quit               = "Quit muTerm",

        /* Font hinting mode names */
        .hint_normal = "normal",
        .hint_light  = "light",
        .hint_mono   = "mono",
        .hint_none   = "none",

        /* Foreground colour preset names */
        .fg_white      = "White",
        .fg_amber      = "Amber",
        .fg_yellow     = "Yellow",
        .fg_green      = "Green",
        .fg_lime       = "Lime",
        .fg_cyan       = "Cyan",
        .fg_light_blue = "Light Blue",
        .fg_blue       = "Blue",
        .fg_pink       = "Pink",
        .fg_magenta    = "Magenta",
        .fg_orange     = "Orange",
        .fg_light_gray = "Light Gray",

        /* Background colour preset names */
        .bg_black       = "Black",
        .bg_dark_gray   = "Dark Gray",
        .bg_charcoal    = "Charcoal",
        .bg_dark_blue   = "Dark Blue",
        .bg_navy        = "Navy",
        .bg_dark_green  = "Dark Green",
        .bg_forest      = "Forest",
        .bg_dark_purple = "Dark Purple",
        .bg_plum        = "Plum",
        .bg_dark_red    = "Dark Red",
        .bg_dark_teal   = "Dark Teal",
        .bg_dark_brown  = "Dark Brown",

        /* Menu navigation hint */
        .menu_hint = "  U/D: navigate   L/R: adjust   A/Start: action   B/Guide: close  ",

        /* Toast notifications */
        .notify_config_saved = "  Config saved  ",
        .notify_config_reset = "  Config reset  ",
};