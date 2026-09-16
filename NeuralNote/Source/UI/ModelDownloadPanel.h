//
// Created by Damien Ronssin on 12.09.26.
//

#ifndef ModelDownloadPanel_h
#define ModelDownloadPanel_h

#include <array>
#include <functional>
#include <memory>

#include <JuceHeader.h>

#include "ModelDownloader.h"
#include "NnFlatButton.h"

class NeuralNoteAudioProcessor;

/**
 * The model panel: a row per size. An installed row is picked by clicking it, and carries the tick
 * when it is the one transcriptions use; a missing one offers its download and shows its progress.
 *
 * Polls rather than being told, because a checkpoint can also arrive or go through the file
 * manager or another instance, and the selection lives in the global settings.
 */
class ModelDownloadPanel
    : public juce::Component
    , private juce::Timer
{
public:
    explicit ModelDownloadPanel(NeuralNoteAudioProcessor& inProcessor);

    static int getIdealWidth();

    static int getIdealHeight();

    /** Called when the models directory goes from holding no checkpoint to holding one, or back. */
    std::function<void()> onInstalledModelsChanged;

    /** Called when the cross is clicked. Hiding the panel is the owner's call. */
    std::function<void()> onCloseRequested;

    void setCloseButtonVisible(bool inVisible);

    void resized() override;

    void paint(juce::Graphics& g) override;

    void mouseMove(const juce::MouseEvent& inEvent) override;

    void mouseExit(const juce::MouseEvent& inEvent) override;

    void mouseUp(const juce::MouseEvent& inEvent) override;

private:
    struct Row {
        explicit Row(ModelSize inModelSize);

        const ModelSize modelSize;
        NnFlatButton downloadButton;
        NnFlatButton cancelButton;
        ModelDownloader::Status status;
        bool isInstalled = false;
        bool isInUse = false;

        // The row's background; everything else in it is laid out inside this.
        juce::Rectangle<int> bounds;
    };

    void timerCallback() override;

    /**
     * Reads every size's status and shows the controls that fit it.
     * @return Whether anything drawn changed.
     */
    bool _updateRows(bool inForce);

    /** @return The index of the row under inPosition, or -1. */
    int _rowAt(juce::Point<int> inPosition) const;

    NeuralNoteAudioProcessor& mProcessor;

    std::array<std::unique_ptr<Row>, ALL_MODEL_SIZES.size()> mRows;

    NnFlatButton mCloseButton {"CloseModelPanel"};
    NnFlatButton mOpenFolderButton {"OpenModelsFolder"};

    bool mHasInstalledModel = false;
    int mHoveredRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModelDownloadPanel)
};

#endif // ModelDownloadPanel_h
