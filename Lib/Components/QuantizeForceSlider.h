//
// Created by Damien Ronssin on 20.03.23.
//

#ifndef HorizontalSlider_h
#define HorizontalSlider_h

#include <JuceHeader.h>
#include "NoteUtils.h"
#include "BasicPitchConstants.h"
#include "UIDefines.h"

class QuantizeForceSlider : public Component
{
public:
    QuantizeForceSlider(RangedAudioParameter& inAttachedValue);

    void resized() override;

    void paint(Graphics& g) override;

    void setTooltip(const String& inTooltip);

private:
    juce::Slider mSlider;
    std::unique_ptr<juce::SliderParameterAttachment> mAttachment;
};

#endif // HorizontalSlider_h
