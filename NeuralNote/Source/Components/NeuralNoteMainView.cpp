//
// Created by Damien Ronssin on 06.03.23.
//

#include "NeuralNoteMainView.h"

NeuralNoteMainView::NeuralNoteMainView(NeuralNoteAudioProcessor& processor)
    : mProcessor(processor)
    , mVisualizationPanel(&processor)
    , mTranscriptionOptions(processor)
    , mNoteOptions(processor)
    , mQuantizePanel(processor)
{
    mProcessor.addListenerToStateValueTree(this);
    jassert(mProcessor.getValueTree().hasProperty(NnId::PlayheadCenteredId));

    mRecordButton = std::make_unique<DrawableButton>("RecordButton", DrawableButton::ButtonStyle::ImageRaw);
    mRecordButton->setClickingTogglesState(true);
    mRecordButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mRecordButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);

    auto record_off_drawable =
        Drawable::createFromImageData(BinaryData::recordingoff_svg, BinaryData::recordingoff_svgSize);
    auto record_on_drawable =
        Drawable::createFromImageData(BinaryData::recordingon_svg, BinaryData::recordingon_svgSize);

    mRecordButton->setImages(
        record_off_drawable.get(), nullptr, nullptr, nullptr, record_on_drawable.get(), nullptr, nullptr);

    mRecordButton->onClick = [this]() {
        bool is_on = mRecordButton->getToggleState();

        // Recording started
        if (is_on) {
            mProcessor.getSourceAudioManager()->startRecording();
        } else {
            // Recording has ended, set processor state to processing
            mProcessor.getSourceAudioManager()->stopRecording();
        }

        updateEnablements();
    };

    mRecordButton->setToggleState(mProcessor.getState() == Recording, NotificationType::dontSendNotification);

    addAndMakeVisible(*mRecordButton);

    mClearButton = std::make_unique<DrawableButton>("ClearButton", DrawableButton::ButtonStyle::ImageRaw);
    mClearButton->setClickingTogglesState(false);
    mClearButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mClearButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);

    auto bin_drawable = Drawable::createFromImageData(BinaryData::deleteicon_svg, BinaryData::deleteicon_svgSize);
    mClearButton->setImages(bin_drawable.get());

    mClearButton->onClick = [this]() {
        mProcessor.clear();
        mVisualizationPanel.clear();
        updateEnablements();
    };
    addAndMakeVisible(*mClearButton);

    mBackButton = std::make_unique<DrawableButton>("BackButton", DrawableButton::ButtonStyle::ImageRaw);
    mBackButton->setClickingTogglesState(false);
    mBackButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mBackButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    auto back_icon_drawable = Drawable::createFromImageData(BinaryData::back_svg, BinaryData::back_svgSize);
    mBackButton->setImages(back_icon_drawable.get());
    mBackButton->onClick = [this]() {
        mProcessor.getPlayer()->reset();
        mPlayPauseButton->setToggleState(false, sendNotification);
        mVisualizationPanel.getAudioMidiViewport().setViewPositionProportionately(0, 0);
    };

    addAndMakeVisible(*mBackButton);

    mPlayPauseButton = std::make_unique<DrawableButton>("PlayPauseButton", DrawableButton::ButtonStyle::ImageRaw);
    mPlayPauseButton->setClickingTogglesState(true);
    mPlayPauseButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mPlayPauseButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    auto play_icon_drawable = Drawable::createFromImageData(BinaryData::play_svg, BinaryData::play_svgSize);
    auto pause_icon_drawable = Drawable::createFromImageData(BinaryData::pause_svg, BinaryData::pause_svgSize);
    mPlayPauseButton->setImages(
        play_icon_drawable.get(), nullptr, nullptr, nullptr, pause_icon_drawable.get(), nullptr, nullptr, nullptr);

    mPlayPauseButton->onClick = [this]() {
        if (mProcessor.getState() == PopulatedAudioAndMidiRegions) {
            mProcessor.getPlayer()->setPlayingState(mPlayPauseButton->getToggleState());
        } else {
            mPlayPauseButton->setToggleState(false, sendNotification);
        }
    };

    addAndMakeVisible(*mPlayPauseButton);

    mCenterButton = std::make_unique<DrawableButton>("PlayPauseButton", DrawableButton::ButtonStyle::ImageRaw);
    mCenterButton->setClickingTogglesState(true);
    mCenterButton->setColour(DrawableButton::ColourIds::backgroundColourId, Colours::transparentBlack);
    mCenterButton->setColour(DrawableButton::ColourIds::backgroundOnColourId, Colours::transparentBlack);
    auto center_icon_drawable_off =
        Drawable::createFromImageData(BinaryData::center_off_svg, BinaryData::center_off_svgSize);
    auto center_icon_drawable_on =
        Drawable::createFromImageData(BinaryData::center_on_svg, BinaryData::center_on_svgSize);
    mCenterButton->setImages(center_icon_drawable_off.get(),
                             nullptr,
                             nullptr,
                             nullptr,
                             center_icon_drawable_on.get(),
                             nullptr,
                             nullptr,
                             nullptr);

    mCenterButton->getToggleStateValue().referTo(
        mProcessor.getValueTree().getPropertyAsValue(NnId::PlayheadCenteredId, nullptr));
    NeuralNoteMainView::valueTreePropertyChanged(mProcessor.getValueTree(), NnId::PlayheadCenteredId);
    addAndMakeVisible(*mCenterButton);

    mMuteButton = std::make_unique<TextButton>("MuteButton");
    mMuteButton->setButtonText("");
    mMuteButton->setClickingTogglesState(true);

    mMuteButton->setColour(TextButton::buttonColourId, Colours::white.withAlpha(0.2f));
    mMuteButton->setColour(TextButton::buttonOnColourId, BLACK);

    mMuteButtonAttachment = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(
        mProcessor.getAPVTS(), ParameterHelpers::getIdStr(ParameterHelpers::MuteId), *mMuteButton);
    addAndMakeVisible(*mMuteButton);

    addAndMakeVisible(mVisualizationPanel);
    addAndMakeVisible(mTranscriptionOptions);
    addAndMakeVisible(mNoteOptions);
    addAndMakeVisible(mQuantizePanel);

    startTimerHz(30);

    updateEnablements();
}

NeuralNoteMainView::~NeuralNoteMainView()
{
    mProcessor.getValueTree().removeListener(this);
    LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void NeuralNoteMainView::resized()
{
    mRecordButton->setBounds(537, 43, 35, 35);
    mClearButton->setBounds(589, 43, 35, 35);
    mBackButton->setBounds(682, 43, 35, 35);
    mPlayPauseButton->setBounds(734, 43, 35, 35);
    mCenterButton->setBounds(786, 43, 35, 35);

    mMuteButton->setBounds(943, 38, 24, 24);

    mVisualizationPanel.setBounds(328, 120, 642, 491);
    mTranscriptionOptions.setBounds(29, 120, 274, 190);
    mNoteOptions.setBounds(29, 334, 274, 133);
    mQuantizePanel.setBounds(29, 491, 274, 120);
}

void NeuralNoteMainView::paint(Graphics& g)
{
    auto background_image = ImageCache::getFromMemory(BinaryData::background_png, BinaryData::background_pngSize);

    g.drawImage(background_image, getLocalBounds().toFloat());
    g.setFont(LABEL_FONT);
    g.drawFittedText("MUTE OUT", Rectangle<int>(939, 63, 31, 23), Justification::centred, 2);
}

void NeuralNoteMainView::timerCallback()
{
    auto processor_state = mProcessor.getState();
    if (mRecordButton->getToggleState() && processor_state != Recording) {
        mRecordButton->setToggleState(false, sendNotification);
        updateEnablements();
    }

    if (mPlayPauseButton->getToggleState() != mProcessor.getPlayer()->isPlaying()) {
        mPlayPauseButton->setToggleState(mProcessor.getPlayer()->isPlaying(), sendNotification);
    }

    if (mPrevState != processor_state) {
        mPrevState = processor_state;
        updateEnablements();
    }
}

void NeuralNoteMainView::repaintPianoRoll()
{
    mVisualizationPanel.repaintPianoRoll();
}

void NeuralNoteMainView::updateEnablements()
{
    auto current_state = mProcessor.getState();
    mPrevState = current_state;

    if (current_state == EmptyAudioAndMidiRegions) {
        mRecordButton->setEnabled(true);
        mClearButton->setEnabled(false);
        mPlayPauseButton->setEnabled(false);
        mBackButton->setEnabled(false);
        mCenterButton->setEnabled(false);
    } else if (current_state == Recording) {
        mRecordButton->setEnabled(true);
        mClearButton->setEnabled(false);
        mPlayPauseButton->setEnabled(false);
        mBackButton->setEnabled(false);
        mCenterButton->setEnabled(false);
    } else if (current_state == Processing) {
        mRecordButton->setEnabled(false);
        mClearButton->setEnabled(false);
        mPlayPauseButton->setEnabled(false);
        mBackButton->setEnabled(false);
        mCenterButton->setEnabled(false);
    } else if (current_state == PopulatedAudioAndMidiRegions) {
        mRecordButton->setEnabled(false);
        mClearButton->setEnabled(true);
        mPlayPauseButton->setEnabled(true);
        mBackButton->setEnabled(true);
        mCenterButton->setEnabled(true);
        mVisualizationPanel.setMidiFileDragComponentVisible();
    }

    repaint();
}

void NeuralNoteMainView::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
    if (property == NnId::PlayheadCenteredId) {
        bool should_center = treeWhosePropertyHasChanged.getProperty(property);
        mCenterButton->setToggleState(should_center, sendNotification);
        mVisualizationPanel.getCombinedAudioMidiRegion().setCenterView(should_center);
    }
}
