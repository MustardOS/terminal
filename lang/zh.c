#include "../lang.h"

const LangDef lang_zh = {

        /* Metadata */
        .lang_code    = "zh",
        .lang_name    = "简体中文",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_term_font_size     = "终端字体大小",
        .item_menu_font_size     = "菜单字体大小",
        .item_font_hinting       = "字体微调",
        .item_fg_colour          = "前景色",
        .item_bg_colour          = "背景色",
        .item_language           = "语言",
        .item_save_config        = "保存配置",
        .item_reset_config       = "重置配置",
        .item_quit               = "退出 muTerm",

        /* Font hinting mode names */
        .hint_normal = "普通",
        .hint_light  = "轻度",
        .hint_mono   = "等宽",
        .hint_none   = "无",

        /* Foreground colour preset names */
        .fg_white      = "白色",
        .fg_amber      = "琥珀色",
        .fg_yellow     = "黄色",
        .fg_green      = "绿色",
        .fg_lime       = "青柠色",
        .fg_cyan       = "青色",
        .fg_light_blue = "浅蓝色",
        .fg_blue       = "蓝色",
        .fg_pink       = "粉色",
        .fg_magenta    = "品红色",
        .fg_orange     = "橙色",
        .fg_light_gray = "浅灰色",

        /* Background colour preset names */
        .bg_black       = "黑色",
        .bg_dark_gray   = "深灰色",
        .bg_charcoal    = "炭灰色",
        .bg_dark_blue   = "深蓝色",
        .bg_navy        = "海军蓝",
        .bg_dark_green  = "深绿色",
        .bg_forest      = "森林绿",
        .bg_dark_purple = "深紫色",
        .bg_plum        = "李子色",
        .bg_dark_red    = "深红色",
        .bg_dark_teal   = "深青色",
        .bg_dark_brown  = "深棕色",

        /* Menu navigation hint */
        .menu_hint = "  上/下: 导航   左/右: 调整   A/Start: 确认   B/Guide: 关闭  ",

        /* Toast notifications */
        .notify_config_saved = "  配置已保存  ",
        .notify_config_reset = "  配置已重置  ",
};
