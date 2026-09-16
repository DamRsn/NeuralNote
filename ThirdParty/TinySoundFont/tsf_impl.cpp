// TinySoundFont's implementation, and the only translation unit that has it.

// Header only: the implementation is compiled separately, as C, in
// stb_vorbis_impl.c. What matters here is that including it defines
// STB_VORBIS_INCLUDE_STB_VORBIS_H, which is the switch that compiles tsf's
// Ogg/.sf3 support in at all. Without it tsf silently ignores the "smpo" chunk
// and tsf_load returns null without even setting an error code -- so the
// soundfont would fail to load with no diagnostic. The static_assert below
// makes that impossible to regress into.
#define STB_VORBIS_HEADER_ONLY
#define STB_VORBIS_NO_STDIO
#include "stb_vorbis.c"

#ifndef STB_VORBIS_INCLUDE_STB_VORBIS_H
    #error "stb_vorbis was not included before tsf.h: tsf would be built without .sf3 support"
#endif

#define TSF_IMPLEMENTATION
#include "tsf.h"

#include "tsf_extras.h"

void tsfExtrasGetLoopingKeys(const tsf* inFont, int inPresetIndex, bool* outKeys)
{
    for (int key = 0; key < 128; key++) {
        outKeys[key] = false;
    }

    if (inFont == nullptr || inPresetIndex < 0 || inPresetIndex >= inFont->presetNum) {
        return;
    }

    const struct tsf_preset& preset = inFont->presets[inPresetIndex];

    for (int i = 0; i < preset.regionNum; i++) {
        const struct tsf_region& region = preset.regions[i];

        // The same test tsf_note_on itself uses to decide whether to loop a voice, so this cannot
        // disagree with what actually happens at playback.
        if (region.loop_mode == TSF_LOOPMODE_NONE || region.loop_start >= region.loop_end) {
            continue;
        }

        for (int key = region.lokey; key <= region.hikey && key < 128; key++) {
            if (key >= 0) {
                outKeys[key] = true;
            }
        }
    }
}
