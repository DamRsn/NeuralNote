#include "UIDefines.h"

UIFonts* UIFonts::fts = nullptr;;

UIFonts& UIFonts::get() {
        if (nullptr == fts) {
            fts = new UIFonts();
        }

        return *fts;
    }
