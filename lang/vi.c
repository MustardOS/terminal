#include "../lang.h"

const LangDef lang_vi = {

        /* Metadata */
        .lang_code    = "vi",
        .lang_name    = "Tiếng Việt",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_term_font_size     = "Cỡ Chữ Terminal",
        .item_menu_font_size     = "Cỡ Chữ Menu",
        .item_font_hinting       = "Làm Nét Chữ",
        .item_fg_colour          = "Màu Chữ",
        .item_bg_colour          = "Màu Nền",
        .item_language           = "Ngôn Ngữ",
        .item_save_config        = "Lưu Cấu Hình",
        .item_reset_config       = "Đặt Lại Cấu Hình",
        .item_quit               = "Thoát muTerm",

        /* Font hinting mode names */
        .hint_normal = "bình thường",
        .hint_light  = "nhẹ",
        .hint_mono   = "mono",
        .hint_none   = "không",

        /* Foreground colour preset names */
        .fg_white      = "Trắng",
        .fg_amber      = "Hổ Phách",
        .fg_yellow     = "Vàng",
        .fg_green      = "Xanh Lá",
        .fg_lime       = "Xanh Chanh",
        .fg_cyan       = "Xanh Lam Lục",
        .fg_light_blue = "Xanh Lam Nhạt",
        .fg_blue       = "Xanh Lam",
        .fg_pink       = "Hồng",
        .fg_magenta    = "Đỏ Tươi",
        .fg_orange     = "Cam",
        .fg_light_gray = "Xám Nhạt",

        /* Background colour preset names */
        .bg_black       = "Đen",
        .bg_dark_gray   = "Xám Đậm",
        .bg_charcoal    = "Than Chì",
        .bg_dark_blue   = "Xanh Lam Đậm",
        .bg_navy        = "Xanh Hải Quân",
        .bg_dark_green  = "Xanh Lá Đậm",
        .bg_forest      = "Xanh Rừng",
        .bg_dark_purple = "Tím Đậm",
        .bg_plum        = "Mận",
        .bg_dark_red    = "Đỏ Đậm",
        .bg_dark_teal   = "Mòng Két Đậm",
        .bg_dark_brown  = "Nâu Đậm",

        /* Menu navigation hint */
        .menu_hint = "  T/D: điều hướng   T/P: điều chỉnh   A/Start: hành động   B/Guide: đóng  ",

        /* Toast notifications */
        .notify_config_saved = "  Đã lưu cấu hình  ",
        .notify_config_reset = "  Đã đặt lại cấu hình  ",
};
