//
// Created by Damien Ronssin on 07.08.26.
//

#include "NnToolbar.h"

#include "NeuralNoteTooltips.h"
#include "NNFileUtils.h"
#include "NnFonts.h"
#include "NnIcons.h"
#include "NnLook.h"
#include "PluginProcessor.h"

namespace
{
constexpr int PADDING_SIDE = 14;
constexpr int GROUP_GAP = 12;
constexpr int PILL_PADDING = 10;
constexpr int PILL_GAP = 8;
constexpr int TEMPO_VALUE_WIDTH = 40;
constexpr int SPINNER_WIDTH = 7;
constexpr int SPINNER_HEIGHT = 4;
constexpr int SPINNER_GAP = 2;
constexpr float LABEL_TRACKING = 0.09f;
} // namespace

NnToolbar::NnToolbar(NeuralNoteAudioProcessor& inProcessor)
    : mProcessor(inProcessor)
    , mDragButton(&inProcessor)
{
    auto tempo_is_valid = [](const juce::String& inText) {
        if (inText.isEmpty()) {
            return false;
        }

        const float tempo = inText.getFloatValue();
        return tempo >= 20.0f && tempo <= 999.0f;
    };

    auto correct_tempo = [](const juce::String& inText) {
        return inText.isEmpty() ? juce::String("120")
                                : juce::String(juce::jlimit(20.0f, 999.0f, inText.getFloatValue()));
    };

    mTempoEditor = std::make_unique<NumericTextEditor<double>>(
        &mProcessor, NnId::ExportTempoId, 6, 120.0, juce::Justification::centredLeft, tempo_is_valid, correct_tempo);
    mTempoEditor->setTooltip(NeuralNoteTooltips::export_tempo);
    addAndMakeVisible(*mTempoEditor);

    mExportButton.setIcon(nn::icons::folderStroked, NnFlatButton::IconStyle::stroked, 13.0f);
    mExportButton.setLabel("Export MIDI out", nn::fonts::buttonLabel());
    mExportButton.setPadding(12, 12, 7);
    mExportButton.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControlAlt);
    mExportButton.setColour(NnFlatButton::iconColourId, nn::colours::textIconSoft);
    mExportButton.setColour(NnFlatButton::textColourId, nn::colours::textButton);
    mExportButton.setTooltip("Write the transcribed MIDI to a file");
    mExportButton.setWantsKeyboardFocus(false);
    mExportButton.onClick = [this] { _exportMidiFile(); };
    addAndMakeVisible(mExportButton);

    mDragButton.setWantsKeyboardFocus(false);
    addAndMakeVisible(mDragButton);

    mClearButton.setIcon(nn::icons::trashStroked, NnFlatButton::IconStyle::stroked, 13.0f);
    mClearButton.setColour(NnFlatButton::backgroundColourId, nn::colours::bgControlAlt);
    mClearButton.setColour(NnFlatButton::iconColourId, nn::colours::textIconSoft);
    mClearButton.setTooltip(NeuralNoteTooltips::clear);
    mClearButton.setWantsKeyboardFocus(false);
    mClearButton.onClick = [this] { mProcessor.clear(); };
    mClearButton.onRightClick = [this] { _showClearMenu(); };
    addAndMakeVisible(mClearButton);

    updateEnablements();
}

void NnToolbar::resized()
{
    auto bounds = getLocalBounds().withTrimmedBottom(1).reduced(PADDING_SIDE, 0);

    const int button_height = nn::metrics::toolbarButton;

    auto place_right = [&bounds](juce::Component& inComponent, int inWidth) {
        inComponent.setBounds(bounds.removeFromRight(inWidth).withSizeKeepingCentre(inWidth, button_height));
        bounds.removeFromRight(GROUP_GAP);
    };

    place_right(mClearButton, button_height);
    place_right(mDragButton, mDragButton.getIdealWidth());
    place_right(mExportButton, mExportButton.getIdealWidth());

    const int label_width =
        juce::roundToInt(std::ceil(nn::trackedTextWidth("EXPORT TEMPO", nn::fonts::pillLabel(), LABEL_TRACKING)));
    const int tempo_width = 2 * PILL_PADDING + label_width + PILL_GAP + TEMPO_VALUE_WIDTH + PILL_GAP + SPINNER_WIDTH;

    mTempoPill = bounds.removeFromRight(tempo_width).withSizeKeepingCentre(tempo_width, button_height);
    mTempoEditor->setBounds(mTempoPill.getX() + PILL_PADDING + label_width + PILL_GAP,
                            mTempoPill.getY() + 6,
                            TEMPO_VALUE_WIDTH,
                            button_height - 12);
}

void NnToolbar::paint(juce::Graphics& g)
{
    g.fillAll(nn::colours::bgRoot);
    nn::drawBottomBorder(g, getLocalBounds(), nn::colours::divSoft);

    _paintTempoPill(g);

    auto filename_area = getLocalBounds()
                             .withTrimmedBottom(1)
                             .reduced(PADDING_SIDE, 0)
                             .withTrimmedRight(getWidth() - mTempoPill.getX() + GROUP_GAP);

    // Nothing at all when there is no file: the waveform's drop zone right below already says the
    // window is empty, and a second sentence saying it is one too many.
    const juce::String filename = mProcessor.getSourceAudioManager()->getDroppedFilename();

    g.setColour(nn::colours::textFile);
    g.setFont(nn::fonts::filename());
    g.drawText(filename, filename_area, juce::Justification::centredLeft, true);
}

void NnToolbar::updateEnablements()
{
    const bool is_finished = mProcessor.getState() == PopulatedAudioAndMidiRegions;

    // Export and drag stay off mid-transcription: a half-finished file gives no sign that it is one.
    mExportButton.setEnabled(is_finished);
    mDragButton.setEnabled(is_finished);
    mTempoEditor->setEnabled(is_finished);
    mTempoEditor->setAlpha(is_finished ? 1.0f : nn::DISABLED_ALPHA);

    // The bin is live as soon as there is anything to throw away, audio with no transcription
    // included. Not while a run is in flight: stopping one is the status bar's cancel, and
    // TranscriptionManager::clear cannot run under a live job.
    const State state = mProcessor.getState();
    mClearButton.setEnabled(state == AudioLoaded || state == PopulatedAudioAndMidiRegions);

    resized();
    repaint();
}

void NnToolbar::_exportMidiFile()
{
    const juce::String filename =
        NNFileUtils::getMidiExportFileName(mProcessor.getSourceAudioManager()->getDroppedFilename());

    mFileChooser = std::make_shared<juce::FileChooser>(
        "Export MIDI", NNFileUtils::getDefaultMidiExportDirectory().getChildFile(filename), "*.mid");

    mFileChooser->launchAsync(
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting,
        [this](const juce::FileChooser& inChooser) {
            const juce::File file = inChooser.getResult();

            if (file == juce::File {}) {
                return;
            }

            const double export_bpm = mProcessor.getValueTree().getProperty(NnId::ExportTempoId, 120.0);
            const auto overflow_mode =
                static_cast<MidiOverflowMode>(static_cast<int>(mProcessor.getValueTree().getProperty(
                    NnId::MidiOverflowModeId, static_cast<int>(MidiOverflowMode::ReuseChannels))));

            const bool success = mMidiFileWriter.writeMidiFile(
                mProcessor.getTranscriptionManager()->getNoteEventVector(),
                file,
                export_bpm,
                mProcessor.getSourceAudioManager()->getExportStartOffsetSeconds(),
                overflow_mode);

            if (!success) {
                juce::NativeMessageBox::showMessageBoxAsync(
                    juce::MessageBoxIconType::NoIcon, "Error", "Could not write the MIDI file.");
            }
        });
}

void NnToolbar::_showClearMenu()
{
    juce::PopupMenu menu;

    auto clear_all = juce::PopupMenu::Item("Clear audio and transcription");
    clear_all.setID(1);
    clear_all.setAction([this] { mProcessor.clear(); });
    menu.addItem(clear_all);

    // Keeps the file loaded and the instrument selection with it, so the obvious next move --
    // adjust the selection and run again -- does not start with re-dropping the audio.
    auto clear_transcription = juce::PopupMenu::Item("Clear transcription only");
    clear_transcription.setID(2);
    clear_transcription.setEnabled(mProcessor.getState() == PopulatedAudioAndMidiRegions);
    clear_transcription.setAction([this] { mProcessor.clearTranscription(); });
    menu.addItem(clear_transcription);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&mClearButton));
}

void NnToolbar::_paintTempoPill(juce::Graphics& g) const
{
    if (mTempoPill.isEmpty()) {
        return;
    }

    // Dimmed rather than absent before there is a transcription to export: the export group holds
    // its place in the toolbar either way, and an empty gap there reads as something failing to
    // draw.
    const float alpha = mTempoEditor->isEnabled() ? 1.0f : nn::DISABLED_ALPHA;

    g.setColour(nn::colours::bgControlAlt.withMultipliedAlpha(alpha));
    g.fillRoundedRectangle(mTempoPill.toFloat(), static_cast<float>(nn::metrics::controlCorner));

    auto content = mTempoPill.reduced(PILL_PADDING, 0);

    g.setColour(nn::colours::textDim.withMultipliedAlpha(alpha));
    nn::drawTrackedText(
        g, "EXPORT TEMPO", nn::fonts::pillLabel(), content.toFloat(), juce::Justification::centredLeft, LABEL_TRACKING);

    // Stacked triangles rather than a spinner control: the value is typed, and these say the field
    // is a number without pretending to be a second way of setting it.
    auto spinner =
        content.removeFromRight(SPINNER_WIDTH).withSizeKeepingCentre(SPINNER_WIDTH, 2 * SPINNER_HEIGHT + SPINNER_GAP);

    g.fillPath(nn::icons::triangleUp(spinner.removeFromTop(SPINNER_HEIGHT).toFloat()));
    spinner.removeFromTop(SPINNER_GAP);
    g.fillPath(nn::icons::triangleDown(spinner.toFloat()));
}
