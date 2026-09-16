//
// Created by Damien Ronssin on 07.08.26.
//

#include "InstrumentInfo.h"

#include <algorithm>
#include <string>

namespace
{
struct GroupDisplay {
    msl::InstrumentGroup group;
    const char* name;
    const char* abbreviation;
    juce::uint32 colour;
};

// The 35 named groups, in enumerator order. Names are shortened where the group's own is longer
// than the 12 px strip can show without ellipsising -- "electric_bass" is the only bass a mix
// usually has, so it is "Bass"; the acoustic one keeps the qualifier that tells them apart.
//
// Colours are per instrument, not per sidebar position, so an instrument keeps its hue as a
// transcription streams in and across two runs of the same file. They are grouped into families
// -- keys blue, guitars green, bass purple, strings pink, brass orange, saxes red-orange, winds
// chartreuse, voice/synth cyan, percussion slate -- with members of a family spread along
// lightness and a few degrees of hue. So the family is legible at a glance and the member is still
// distinguishable: no two of the 35 are within a CIELAB distance of 6, and all sit well clear of
// the piano roll's background.
constexpr GroupDisplay GROUP_DISPLAYS[] = {
    // Keys
    {msl::InstrumentGroup::AcousticPiano, "Piano", "PNO", 0x3372ff},
    {msl::InstrumentGroup::ElectricPiano, "Electric Piano", "EPN", 0x85a0ff},
    {msl::InstrumentGroup::Organ, "Organ", "ORG", 0x1f5fc1},
    // Guitars
    {msl::InstrumentGroup::AcousticGuitar, "Acoustic Guitar", "AGT", 0x45d1a8},
    {msl::InstrumentGroup::CleanElectricGuitar, "Electric Guitar", "GTR", 0x7aeed6},
    {msl::InstrumentGroup::DistortedElectricGuitar, "Distorted Guitar", "DGT", 0x388d6d},
    // Bass
    {msl::InstrumentGroup::ElectricBass, "Bass", "BAS", 0x9033ff},
    {msl::InstrumentGroup::AcousticBass, "Acoustic Bass", "ABS", 0xc685ff},
    {msl::InstrumentGroup::Contrabass, "Contrabass", "CBS", 0x5b1fc1},
    // Strings
    {msl::InstrumentGroup::Violin, "Violin", "VLN", 0xf9295d},
    {msl::InstrumentGroup::Viola, "Viola", "VLA", 0xff4363},
    {msl::InstrumentGroup::Cello, "Cello", "VLC", 0xea175e},
    {msl::InstrumentGroup::StringEnsemble, "Strings", "STR", 0xff6471},
    {msl::InstrumentGroup::SynthStrings, "Synth Strings", "SST", 0xc11f63},
    {msl::InstrumentGroup::OrchestralHarp, "Harp", "HRP", 0xff8585},
    // Brass
    {msl::InstrumentGroup::Trumpet, "Trumpet", "TPT", 0xffc458},
    {msl::InstrumentGroup::Trombone, "Trombone", "TBN", 0xe07f25},
    {msl::InstrumentGroup::FrenchHorn, "French Horn", "HRN", 0xf2a33c},
    {msl::InstrumentGroup::BrassSection, "Brass", "BRS", 0xffdd81},
    {msl::InstrumentGroup::Tuba, "Tuba", "TBA", 0xb46029},
    // Saxes
    {msl::InstrumentGroup::SopranoAndAltoSax, "Alto Sax", "ASX", 0xfeac85},
    {msl::InstrumentGroup::TenorSax, "Tenor Sax", "TSX", 0xe8704a},
    {msl::InstrumentGroup::BaritoneSax, "Baritone Sax", "BSX", 0xae4532},
    // Winds
    {msl::InstrumentGroup::Flutes, "Flute", "FLT", 0xc3d94e},
    {msl::InstrumentGroup::Oboe, "Oboe", "OBO", 0xc9e868},
    {msl::InstrumentGroup::EnglishHorn, "English Horn", "EHN", 0xd0f385},
    {msl::InstrumentGroup::Clarinet, "Clarinet", "CLR", 0xbbc638},
    {msl::InstrumentGroup::Bassoon, "Bassoon", "BSN", 0x9c9c39},
    // Voice and synth
    {msl::InstrumentGroup::Voice, "Voice", "VOX", 0x4bc1e7},
    {msl::InstrumentGroup::SynthLead, "Synth Lead", "LED", 0x86d7fe},
    {msl::InstrumentGroup::SynthPad, "Synth Pad", "PAD", 0x329bae},
    // Percussion
    {msl::InstrumentGroup::Drums, "Drums", "DRM", 0x77869f},
    {msl::InstrumentGroup::Timpani, "Timpani", "TMP", 0x949fb9},
    {msl::InstrumentGroup::ChromaticPercussion, "Chromatic Perc.", "CPR", 0xb3b9d1},
    {msl::InstrumentGroup::OrchestraHit, "Orchestra Hit", "OHT", 0x616f80},
};

static_assert(std::size(GROUP_DISPLAYS) == 35, "One entry per named group in msl::InstrumentGroup");

/**
 * The point of the table above is that no two instruments share a colour, and nothing else in the
 * program enforces that -- a duplicate would simply make two strips indistinguishable. Checked
 * here so a future edit that reuses a value fails the build rather than shipping.
 */
constexpr bool allColoursDistinct()
{
    for (std::size_t i = 0; i < std::size(GROUP_DISPLAYS); i++) {
        for (std::size_t j = i + 1; j < std::size(GROUP_DISPLAYS); j++) {
            if (GROUP_DISPLAYS[i].colour == GROUP_DISPLAYS[j].colour) {
                return false;
            }
        }
    }

    return true;
}

static_assert(allColoursDistinct(), "Two instruments share a colour");

/** The same, for the groups themselves: a typo that duplicated one would shadow the other. */
constexpr bool allGroupsDistinct()
{
    for (std::size_t i = 0; i < std::size(GROUP_DISPLAYS); i++) {
        for (std::size_t j = i + 1; j < std::size(GROUP_DISPLAYS); j++) {
            if (GROUP_DISPLAYS[i].group == GROUP_DISPLAYS[j].group) {
                return false;
            }
        }
    }

    return true;
}

static_assert(allGroupsDistinct(), "Two entries name the same instrument group");

const GroupDisplay* findDisplay(msl::InstrumentGroup inGroup)
{
    const auto it = std::find_if(std::begin(GROUP_DISPLAYS),
                                 std::end(GROUP_DISPLAYS),
                                 [inGroup](const GroupDisplay& inDisplay) { return inDisplay.group == inGroup; });

    return it == std::end(GROUP_DISPLAYS) ? nullptr : it;
}

const GroupDisplay* findDisplayForProgram(int inProgram)
{
    const std::optional<msl::InstrumentGroup> group = msl::instrumentGroupFor(inProgram);

    return group.has_value() ? findDisplay(*group) : nullptr;
}
} // namespace

InstrumentDisplay instrumentDisplayFor(int inProgram)
{
    if (const GroupDisplay* display = findDisplayForProgram(inProgram)) {
        return {display->name, display->abbreviation};
    }

    // An unnamed singleton group. The library's own label is the honest thing to show, and its
    // program number is the only distinguishing part, so that is what the chip carries.
    return {juce::String(msl::instrumentLabel(inProgram)), juce::String(inProgram)};
}

InstrumentDisplay instrumentDisplayForGroup(msl::InstrumentGroup inGroup)
{
    const GroupDisplay* display = findDisplay(inGroup);

    // Every enumerator has a row, which the static_assert above is what keeps true.
    jassert(display != nullptr);

    return display != nullptr ? InstrumentDisplay {display->name, display->abbreviation}
                              : InstrumentDisplay {juce::String(std::string(msl::instrumentName(inGroup))), "???"};
}

juce::Colour instrumentColourFor(int inProgram)
{
    if (const GroupDisplay* display = findDisplayForProgram(inProgram)) {
        return juce::Colour(display->colour).withAlpha(1.0f);
    }

    // An unnamed singleton group -- decodable, but never something the model picks. A fixed hue
    // spun by the program number keeps it stable and keeps it from passing for a named instrument.
    return juce::Colour::fromHSL(static_cast<float>(inProgram % 128) / 128.0f, 0.22f, 0.62f, 1.0f);
}

juce::Colour instrumentColourForGroup(msl::InstrumentGroup inGroup)
{
    const GroupDisplay* display = findDisplay(inGroup);

    jassert(display != nullptr);

    return display != nullptr ? juce::Colour(display->colour).withAlpha(1.0f) : juce::Colours::grey;
}
