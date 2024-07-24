#include "PluginProcessor.h"
#include "PluginEditor.h"

NeuralNoteAudioProcessor::NeuralNoteAudioProcessor()
    : mAPVTS(*this, nullptr, NnId::ParametersId, ParameterHelpers::createParameterLayout())
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

    auto is_mute = mParams[ParameterHelpers::MuteId]->getValue() > 0.5f;

    if (is_mute) {
        buffer.clear();
    }

    mPlayer->processBlock(buffer);
}

juce::AudioProcessorEditor* NeuralNoteAudioProcessor::createEditor()
{
    return new NeuralNoteEditor(*this);
}

void NeuralNoteAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto full_state_tree = ValueTree(NnId::FullStateId);

    full_state_tree.setProperty(NnId::NeuralNoteVersionId, ProjectInfo::versionString, nullptr);

    // PARAMETERS
    auto apvts = mAPVTS.copyState();
    jassert(apvts.getType() == NnId::ParametersId);

    full_state_tree.appendChild(apvts, nullptr);

    full_state_tree.appendChild(mValueTree, nullptr);

    std::unique_ptr<XmlElement> xml(full_state_tree.createXml());

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
        ValueTree full_state_tree = ValueTree::fromXml(*xmlState);

        if (full_state_tree.isValid() && full_state_tree.hasType(NnId::FullStateId)) {
            // Extract the parameters ValueTree
            auto parameter_tree = full_state_tree.getChildWithName(NnId::ParametersId);

            ParameterHelpers::updateParametersFromState(parameter_tree, mParams);

        } else {
            jassertfalse;
        }

        if (full_state_tree.isValid() && full_state_tree.hasType(NnId::FullStateId)) {
            auto new_value_tree = full_state_tree.getChildWithName(NnId::NeuralNoteStateId);
            _updateValueTree(new_value_tree);
        } else {
            jassertfalse;
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
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getBpm().hasValue()) {
        return std::to_string(static_cast<int>(std::round(*mPlayheadInfoStartRecord->getBpm())));
    }

    if (mCurrentTempo > 0) {
        return std::to_string(static_cast<int>(std::round(mCurrentTempo.load())));
    }

    return "-";
}

std::string NeuralNoteAudioProcessor::getTimeSignatureStr() const
{
    if (mPlayheadInfoStartRecord.hasValue() && mPlayheadInfoStartRecord->getTimeSignature().hasValue()) {
        int num = mPlayheadInfoStartRecord->getTimeSignature()->numerator;
        int denom = mPlayheadInfoStartRecord->getTimeSignature()->denominator;
        return std::to_string(num) + " / " + std::to_string(denom);
    }

    if (mCurrentTimeSignatureNum > 0 && mCurrentTimeSignatureDenom > 0) {
        return std::to_string(mCurrentTimeSignatureNum.load()) + " / "
               + std::to_string(mCurrentTimeSignatureDenom.load());
    }

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

void NeuralNoteAudioProcessor::addListenerToStateValueTree(juce::ValueTree::Listener* inListener)
{
    mValueTree.addListener(inListener);
}

ValueTree NeuralNoteAudioProcessor::_createDefaultValueTree()
{
    ValueTree default_value_tree(NnId::NeuralNoteStateId);

    for (const auto& [id, default_value]: NnId::OrderedStatePropertiesWithDefault) {
        default_value_tree.setProperty(id, default_value, nullptr);
    }

    return default_value_tree;
}

void NeuralNoteAudioProcessor::_updateValueTree(const ValueTree& inNewState)
{
    jassert(inNewState.getType() == NnId::NeuralNoteStateId);
    jassert(mValueTree.getNumProperties() == static_cast<int>(NnId::OrderedStatePropertiesWithDefault.size()));

    if (inNewState.isValid()) {
        // Set all properties from inNewState to mValueTree, ignoring extra ones if any.
        // If less, missing properties will be left as is.
        for (const auto& [prop_id, default_val]: NnId::OrderedStatePropertiesWithDefault) {
            if (inNewState.hasProperty(prop_id)) {
                mValueTree.setProperty(prop_id, inNewState.getProperty(prop_id), nullptr);
            }
        }
    } else {
        jassertfalse;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralNoteAudioProcessor();
}
