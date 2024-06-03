#include "PluginProcessor.h"
#include "PluginEditor.h"

NeuralNoteAudioProcessor::NeuralNoteAudioProcessor()
    : mAPVTS(*this, nullptr, "PARAMETERS", ParameterHelpers::createParameterLayout())
{
    for (size_t i = 0; i < mParams.size(); i++) {
        auto pid = static_cast<ParameterHelpers::ParamIdEnum>(i);
        mParams[i] = mAPVTS.getParameter(ParameterHelpers::getIdStr(pid));
    }

    mSourceAudioManager = std::make_unique<SourceAudioManager>(this);
    mPlayer = std::make_unique<Player>(this);
    mTranscriptionManager = std::make_unique<TranscriptionManager>(this);
}

void NeuralNoteAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSourceAudioManager->prepareToPlay(sampleRate, samplesPerBlock);
    mPlayer->prepareToPlay(sampleRate, samplesPerBlock);
}

void NeuralNoteAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    // Get tempo and time signature for UI.
    auto playhead_info = getPlayHead()->getPosition();
    if (playhead_info.hasValue()) {
        if (playhead_info->getBpm().hasValue())
            mCurrentTempo = *playhead_info->getBpm();
        if (playhead_info->getTimeSignature().hasValue()) {
            mCurrentTimeSignatureNum = playhead_info->getTimeSignature()->numerator;
            mCurrentTimeSignatureDenom = playhead_info->getTimeSignature()->denominator;
        }
    }

    mSourceAudioManager->processBlock(buffer);

    if (mState.load() == Recording) {
        if (!mWasRecording) {
            mWasRecording = true;
            mPlayheadInfoStartRecord = getPlayHead()->getPosition();
            mTranscriptionManager->getRhythmOptions().setInfo(false, mPlayheadInfoStartRecord);
        }
    } else {
        // If we were previously recording but not anymore (user clicked record button to stop it).
        if (mWasRecording) {
            mWasRecording = false;
        }
    }

    auto isMute = mParams[ParameterHelpers::MuteId]->getValue() > 0.5f;

    if (isMute)
        buffer.clear();

    mPlayer->processBlock(buffer);
}

juce::AudioProcessorEditor* NeuralNoteAudioProcessor::createEditor()
{
    return new NeuralNoteEditor(*this);
}

void NeuralNoteAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state_tree = ValueTree("NeuralNoteState");

    state_tree.setProperty("Version", ProjectInfo::versionString, nullptr);

    // PARAMETERS
    auto apvts = mAPVTS.copyState();
    jassert(apvts.getType() == Identifier("PARAMETERS"));

    state_tree.appendChild(apvts, nullptr);

    std::unique_ptr<XmlElement> xml(state_tree.createXml());

    if (xml != nullptr) {
        copyXmlToBinary(*xml, destData);
    }
}

void NeuralNoteAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Create an XmlElement from the binary data
    std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr) {
        // Convert XmlElement to ValueTree
        ValueTree stateTree = ValueTree::fromXml(*xmlState);

        if (stateTree.isValid() && stateTree.hasType(Identifier("NeuralNoteState"))) {
            // Extract the parameters ValueTree
            auto apvtsState = stateTree.getChildWithName(Identifier("PARAMETERS"));

            if (apvtsState.isValid()) {
                // Iterate through the properties in the loaded state
                for (int i = 0; i < apvtsState.getNumChildren(); ++i) {
                    auto child = apvtsState.getChild(i);

                    if (child.isValid() && child.hasProperty("id") && child.hasProperty("value")) {
                        auto param_id = child.getProperty("id").toString();

                        int index = ParameterHelpers::ParamIdStr.indexOf(param_id);

                        if (index >= 0) {
                            auto* param = mParams[index];
                            auto value = jlimit(param->getNormalisableRange().start,
                                                param->getNormalisableRange().end,
                                                static_cast<float>(child.getProperty("value")));

                            auto norm_value = param->getNormalisableRange().convertTo0to1(value);
                            param->setValueNotifyingHost(norm_value);
                        }
                    }
                }
            }
        }
    }
}

void NeuralNoteAudioProcessor::clear()
{
    mPlayheadInfoStartRecord = juce::Optional<juce::AudioPlayHead::PositionInfo>();

    mCurrentTempo = -1;
    mCurrentTimeSignatureNum = -1;
    mCurrentTimeSignatureDenom = -1;

    mWasRecording = false;

    mPlayer->reset();
    mSourceAudioManager->clear();
    mTranscriptionManager->clear();

    mState.store(EmptyAudioAndMidiRegions);
}

const juce::Optional<juce::AudioPlayHead::PositionInfo>& NeuralNoteAudioProcessor::getPlayheadInfoOnRecordStart()
{
    return mPlayheadInfoStartRecord;
}

bool NeuralNoteAudioProcessor::canQuantize() const
{
    return mTranscriptionManager->getRhythmOptions().canPerformQuantization();
}

std::string NeuralNoteAudioProcessor::getTempoStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getBpm().hasValue())
        return std::to_string(static_cast<int>(std::round(*mPlayheadInfoStartRecord->getBpm())));
    else if (mCurrentTempo > 0)
        return std::to_string(static_cast<int>(std::round(mCurrentTempo.load())));
    else
        return "-";
}

std::string NeuralNoteAudioProcessor::getTimeSignatureStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getTimeSignature().hasValue()) {
        int num = mPlayheadInfoStartRecord->getTimeSignature()->numerator;
        int denom = mPlayheadInfoStartRecord->getTimeSignature()->denominator;
        return std::to_string(num) + " / " + std::to_string(denom);
    } else if (mCurrentTimeSignatureNum > 0 && mCurrentTimeSignatureDenom > 0)
        return std::to_string(mCurrentTimeSignatureNum.load()) + " / "
               + std::to_string(mCurrentTimeSignatureDenom.load());
    else
        return "- / -";
}

SourceAudioManager* NeuralNoteAudioProcessor::getSourceAudioManager()
{
    return mSourceAudioManager.get();
}

Player* NeuralNoteAudioProcessor::getPlayer()
{
    return mPlayer.get();
}

TranscriptionManager* NeuralNoteAudioProcessor::getTranscriptionManager()
{
    return mTranscriptionManager.get();
}

std::array<RangedAudioParameter*, ParameterHelpers::TotalNumParams>& NeuralNoteAudioProcessor::getParams()
{
    return mParams;
}

float NeuralNoteAudioProcessor::getParameterValue(ParameterHelpers::ParamIdEnum inParamId) const
{
    return ParameterHelpers::getUnmappedParamValue(mParams[inParamId]);
}

NeuralNoteMainView* NeuralNoteAudioProcessor::getNeuralNoteMainView() const
{
    auto* editor = dynamic_cast<NeuralNoteEditor*>(getActiveEditor());

    if (editor != nullptr) {
        return editor->getMainView();
    }

    return nullptr;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralNoteAudioProcessor();
}
