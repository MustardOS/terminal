#include "../lang.h"

const LangDef lang_es = {

        /* Metadata */
        .lang_code    = "es",
        .lang_name    = "Español",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_term_font_size     = "Tamaño de Fuente Terminal",
        .item_menu_font_size     = "Tamaño de Fuente del Menú",
        .item_font_hinting       = "Suavizado de Fuente",
        .item_fg_colour          = "Color de Texto",
        .item_bg_colour          = "Color de Fondo",
        .item_language           = "Idioma",
        .item_save_config        = "Guardar Config",
        .item_reset_config       = "Restablecer Config",
        .item_quit               = "Salir de muTerm",

        /* Font hinting mode names */
        .hint_normal = "normal",
        .hint_light  = "suave",
        .hint_mono   = "mono",
        .hint_none   = "ninguno",

        /* Foreground colour preset names */
        .fg_white      = "Blanco",
        .fg_amber      = "Ámbar",
        .fg_yellow     = "Amarillo",
        .fg_green      = "Verde",
        .fg_lime       = "Lima",
        .fg_cyan       = "Cian",
        .fg_light_blue = "Azul Claro",
        .fg_blue       = "Azul",
        .fg_pink       = "Rosa",
        .fg_magenta    = "Magenta",
        .fg_orange     = "Naranja",
        .fg_light_gray = "Gris Claro",

        /* Background colour preset names */
        .bg_black       = "Negro",
        .bg_dark_gray   = "Gris Oscuro",
        .bg_charcoal    = "Carbón",
        .bg_dark_blue   = "Azul Oscuro",
        .bg_navy        = "Azul Marino",
        .bg_dark_green  = "Verde Oscuro",
        .bg_forest      = "Bosque",
        .bg_dark_purple = "Morado Oscuro",
        .bg_plum        = "Ciruela",
        .bg_dark_red    = "Rojo Oscuro",
        .bg_dark_teal   = "Verde Azulado Oscuro",
        .bg_dark_brown  = "Marrón Oscuro",

        /* Menu navigation hint */
        .menu_hint = "  A/B: navegar   L/R: ajustar   A/Start: acción   B/Guía: cerrar  ",

        /* Toast notifications */
        .notify_config_saved = "  Config guardada  ",
        .notify_config_reset = "  Config restablecida  ",

};