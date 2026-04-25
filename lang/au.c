#include "../lang.h"

const LangDef lang_au = {

        /* Metadata */
        .lang_code    = "au",
        .lang_name    = "Australian",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_term_font_size     = "Terminal Font Size, Mate",
        .item_menu_font_size     = "Menu Font Size",
        .item_font_hinting       = "Font Hinting",
        .item_fg_colour          = "Foreground Colour",
        .item_bg_colour          = "Background Colour",
        .item_language           = "Language",
        .item_save_config        = "Save Config",
        .item_reset_config       = "Reset Config",
        .item_quit               = "Chuck a Sickie",

        /* Font hinting mode names */
        .hint_normal = "no worries",
        .hint_light  = "easy",
        .hint_mono   = "mono",
        .hint_none   = "nah",

        /* Foreground colour preset names */
        .fg_white      = "White as",
        .fg_amber      = "Amber (Like Ya Beer)",
        .fg_yellow     = "Yellow",
        .fg_green      = "Green",
        .fg_lime       = "Lime",
        .fg_cyan       = "Cyan",
        .fg_light_blue = "Blue Sky",
        .fg_blue       = "Blue",
        .fg_pink       = "Pink",
        .fg_magenta    = "Magenta",
        .fg_orange     = "Orange (Go Socceroos)",
        .fg_light_gray = "Light Grey",

        /* Background colour preset names */
        .bg_black       = "Black",
        .bg_dark_gray   = "Dark Grey",
        .bg_charcoal    = "Charcoal",
        .bg_dark_blue   = "Dark Blue",
        .bg_navy        = "Navy",
        .bg_dark_green  = "Dark Green",
        .bg_forest      = "Bush",
        .bg_dark_purple = "Dark Purple",
        .bg_plum        = "Plum",
        .bg_dark_red    = "Dark Red",
        .bg_dark_teal   = "Dark Teal",
        .bg_dark_brown  = "Outback Brown",

        /* Menu navigation hint */
        .menu_hint = "  U/D: navigate   L/R: adjust   A/Start: yeah nah   B/Guide: rack off  ",

        /* Toast notifications */
        .notify_config_saved = "  Config saved, ripper!  ",
        .notify_config_reset = "  Config reset, she'll be right  ",
};
