#include "../lang.h"

const LangDef lang_el = {

        /* Metadata */
        .lang_code    = "el",
        .lang_name    = "Ελληνικά",
        .lang_author  = "MustardOS",
        .lang_version = "1.0.0",

        /* Menu item labels */
        .item_term_font_size     = "Μέγεθος Γραμματοσειράς Terminal",
        .item_menu_font_size     = "Μέγεθος Γραμματοσειράς Μενού",
        .item_font_hinting       = "Ευθυγράμμιση Γραμματοσειράς",
        .item_fg_colour          = "Χρώμα Προσκηνίου",
        .item_bg_colour          = "Χρώμα Φόντου",
        .item_language           = "Γλώσσα",
        .item_save_config        = "Αποθήκευση Ρυθμίσεων",
        .item_reset_config       = "Επαναφορά Ρυθμίσεων",
        .item_quit               = "Έξοδος από muTerm",

        /* Font hinting mode names */
        .hint_normal = "κανονικό",
        .hint_light  = "ελαφρύ",
        .hint_mono   = "μονοδιάστατο",
        .hint_none   = "κανένα",

        /* Foreground colour preset names */
        .fg_white      = "Λευκό",
        .fg_amber      = "Κεχριμπάρι",
        .fg_yellow     = "Κίτρινο",
        .fg_green      = "Πράσινο",
        .fg_lime       = "Λαχανί",
        .fg_cyan       = "Κυανό",
        .fg_light_blue = "Γαλάζιο",
        .fg_blue       = "Μπλε",
        .fg_pink       = "Ροζ",
        .fg_magenta    = "Ματζέντα",
        .fg_orange     = "Πορτοκαλί",
        .fg_light_gray = "Ανοιχτό Γκρι",

        /* Background colour preset names */
        .bg_black       = "Μαύρο",
        .bg_dark_gray   = "Σκούρο Γκρι",
        .bg_charcoal    = "Ανθρακί",
        .bg_dark_blue   = "Σκούρο Μπλε",
        .bg_navy        = "Ναυτικό Μπλε",
        .bg_dark_green  = "Σκούρο Πράσινο",
        .bg_forest      = "Δασικό Πράσινο",
        .bg_dark_purple = "Σκούρο Μωβ",
        .bg_plum        = "Δαμάσκηνο",
        .bg_dark_red    = "Σκούρο Κόκκινο",
        .bg_dark_teal   = "Σκούρο Πράσινο-Μπλε",
        .bg_dark_brown  = "Σκούρο Καφέ",

        /* Menu navigation hint */
        .menu_hint = "  Π/Κ: πλοήγηση   Α/Δ: ρύθμιση   A/Start: ενέργεια   B/Guide: κλείσιμο  ",

        /* Toast notifications */
        .notify_config_saved = "  Οι ρυθμίσεις αποθηκεύτηκαν  ",
        .notify_config_reset = "  Οι ρυθμίσεις επαναφέρθηκαν  ",
};
