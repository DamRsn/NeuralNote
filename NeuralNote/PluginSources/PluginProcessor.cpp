#include "PluginProcessor.h"
#include "PluginEditor.h"

NeuralNoteAudioProcessor::NeuralNoteAudioProcessor()
    : mAPVTS(*this, nullptr, NnId::ParametersId, ParameterHelpers::createParameterLayout())
{
    // Enable logging or not
#if 0
    mLogger.reset(FileLogger::createDefaultAppLogger("/tmp/NeuralNote", "log.txt", "YO! \n"));
    Logger::setCurrentLogger(mLogger.get());
#endif

    for (size_t i = 0; i < mParams.size(); i++) {
        auto pid = static_cast<ParameterHelpers::ParamIdEnum>(i);
        mParams[i] = mAPVTS.getParameter(ParameterHelpers::getIdStr(pid));
    }

    mSourceAudioManager = std::make_unique<SourceAudioManager>(this);
    mPlayer = std::make_unique<Player>(this);
    mTranscriptionManager = std::make_unique<TranscriptionManager>(this);

    // After mPlayer: it pushes every fader it holds into that player's synth.
    mInstrumentMixer = std::make_unique<InstrumentMixer>(this);
}

NeuralNoteAudioProcessor::~NeuralNoteAudioProcessor()
{
    Logger::setCurrentLogger(nullptr);
}

void NeuralNoteAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSourceAudioManager->prepareToPlay(sampleRate, samplesPerBlock);
    mPlayer->prepareToPlay(sampleRate, samplesPerBlock);
}

void NeuralNoteAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    mSourceAudioManager->processBlock(buffer);

    auto is_mute = mParams[ParameterHelpers::MuteId]->getValue() > 0.5f;

    if (is_mute) {
        buffer.clear();
    }

    mPlayer->processBlock(buffer, midiMessages);
}

AudioProcessorEditor* NeuralNoteAudioProcessor::createEditor()
{
    return new NeuralNoteEditor(*this);
}

void NeuralNoteAudioProcessor::getStateInformation(MemoryBlock& destData)
{
    auto full_state_tree = ValueTree(NnId::FullStateId);

    full_state_tree.setProperty(NnId::NeuralNoteVersionId, ProjectInfo::versionString, nullptr);

    // PARAMETERS
    auto apvts = mAPVTS.copyState();
    jassert(apvts.getType() == NnId::ParametersId);

    full_state_tree.appendChild(apvts, nullptr);

    // NEURAL NOTE STATE
    // Update value tree with current state
    mPlayer->saveStateToValueTree();

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
    mPlayer->reset();
    mSourceAudioManager->clear();
    mTranscriptionManager->clear();

    mState.store(EmptyAudioAndMidiRegions);

    // Every clear path has to reset the view too, not just the clear button: a cancelled or failed
    // transcription otherwise leaves the audio region sized and scrolled for audio that is gone.
    if (auto* main_view = getNeuralNoteMainView()) {
        main_view->clear();
    }
}

void NeuralNoteAudioProcessor::clearTranscription()
{
    mPlayer->reset();
    mTranscriptionManager->clear();

    // Falls back to Empty rather than asserting: a run that failed before any audio was acquired
    // has nothing to go back to.
    mState.store(mSourceAudioManager->getNumSamplesDownAcquired() > 0 ? AudioLoaded : EmptyAudioAndMidiRegions);

    // Re-sizes the audio region from the current sample count, which is unchanged here -- so this
    // is the same call as clear()'s and it leaves the waveform where it is.
    if (auto* main_view = getNeuralNoteMainView()) {
        main_view->clear();
    }
}

SourceAudioManager* NeuralNoteAudioProcessor::getSourceAudioManager() const
{
    return mSourceAudioManager.get();
}

Player* NeuralNoteAudioProcessor::getPlayer() const
{
    return mPlayer.get();
}

TranscriptionManager* NeuralNoteAudioProcessor::getTranscriptionManager() const
{
    return mTranscriptionManager.get();
}

InstrumentMixer* NeuralNoteAudioProcessor::getInstrumentMixer() const
{
    return mInstrumentMixer.get();
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

AudioProcessorValueTreeState& NeuralNoteAudioProcessor::getAPVTS()
{
    return mAPVTS;
}

ValueTree& NeuralNoteAudioProcessor::getValueTree()
{
    return mValueTree;
}

void NeuralNoteAudioProcessor::addListenerToStateValueTree(ValueTree::Listener* inListener)
{
    mValueTree.addListener(inListener);
}

void NeuralNoteAudioProcessor::removeListenerFromStateValueTree(ValueTree::Listener* inListener)
{
    mValueTree.removeListener(inListener);
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

        // Children are not covered by the loop above, and the instrument mixer is one. Replaced
        // wholesale rather than merged: the saved set of instruments is the authority, and a stale
        // node left behind would keep muting an instrument the reloaded session no longer has.
        if (const auto saved_mixer = inNewState.getChildWithName(NnId::InstrumentMixerId); saved_mixer.isValid()) {
            mValueTree.removeChild(mValueTree.getChildWithName(NnId::InstrumentMixerId), nullptr);
            mValueTree.appendChild(saved_mixer.createCopy(), nullptr);
        }
    } else {
        jassertfalse;
    }
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NeuralNoteAudioProcessor();
}
