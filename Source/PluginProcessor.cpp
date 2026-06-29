/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogToMidiSynthAudioProcessor::AnalogToMidiSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // osc message connection check
    if ( !connect(7562) ) {
        showConnectionErrorMessage("Error: Could not connect to UDP Port");
    }
    
    // adds listeners for the note on and note off messages
    OSCReceiver::addListener(this, "/bela/midi/noteOn");
    OSCReceiver::addListener(this, "/bela/midi/noteOff");
}

// this shows the osc connection error
void AnalogToMidiSynthAudioProcessor::showConnectionErrorMessage ( const juce::String& messageText){
    auto options = juce::MessageBoxOptions::makeOptionsOk (juce::MessageBoxIconType::WarningIcon, "Connection error", messageText);
    messageBox = juce::AlertWindow::showScopedAsync (options, nullptr );
}

// this sets up all of the parameters that come from the UI
// used in the constructor
juce::AudioProcessorValueTreeState::ParameterLayout AnalogToMidiSynthAudioProcessor::createParameterLayout() {
        return {
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "cutoff", 1 }, "Cutoff Frequency",
                    juce::NormalisableRange<float> (20.0f, 20000.0f), 440.0f),
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "resonance", 1 }, "Resonance",
                    juce::NormalisableRange<float> (0.0f, 10.0f), 2.0f),
            std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "waveform", 1 }, "Waveform", juce::StringArray { "Sine", "Sawtooth", "Triangle" }, 0),
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "attack", 1 }, "Attack", juce::NormalisableRange<float> (0.01f, 5.0f), 0.1f),
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "decay", 1 }, "Decay", juce::NormalisableRange<float> (0.01f, 5.0f), 0.1f),
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "sustain", 1 }, "Sustain", juce::NormalisableRange<float> (0.0f, 1.0f), 1.0f),
            std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "release", 1 }, "Release", juce::NormalisableRange<float> (0.01f, 5.0f), 0.1f)
            
        };
}


AnalogToMidiSynthAudioProcessor::~AnalogToMidiSynthAudioProcessor()
{
}


//==============================================================================
const juce::String AnalogToMidiSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogToMidiSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AnalogToMidiSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AnalogToMidiSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AnalogToMidiSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalogToMidiSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AnalogToMidiSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogToMidiSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AnalogToMidiSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void AnalogToMidiSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AnalogToMidiSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // reset our modulo counter
    phase = 0.0f;
    phaseDelta = 0.0f;
    level = 0.0f;
    
    // this sets up the low pass filter object
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = currentSampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getMainBusNumOutputChannels();
    lpf.reset();
    updateFilter();
    lpf.prepare(spec);
    
    // gives the current sample rate to the ADSR class object
    adsr.setSampleRate(currentSampleRate);
}

void AnalogToMidiSynthAudioProcessor::oscMessageReceived(const juce::OSCMessage& message){
    
    DBG("Received OSC Message: " + message.getAddressPattern().toString());
    
    if ( message.size() == 2 && message[0].isInt32() && message[1].isInt32()) {
        // getting the midi messages from the OSC message
        float midiNote = message[0].getInt32();
        float velocity = message[1].getInt32();
        
        // calculating the frequency based on the midi note number
        float freq = 440.0f * std::pow(2.0f, (midiNote - 69.0f) / 12.0f);
        float level = (float)velocity / 127.0f;
        
        // handling when the note is signaled to be off
        // also handles setting the thread saftey flags
        if (message.getAddressPattern().toString() == "/bela/midi/noteOff") {
            DBG("  ---> JUCE Executing Note OFF");
            triggerNoteOff = true;
        }
        else {
            // note is on, store the variables here
            
            DBG("  ---> JUCE Executing Note ON: MIDI " + juce::String(midiNote) + " | Vel: " + juce::String(velocity));
            
            targetFrequency.store(freq);
            targetLevel.store(level);
            triggerNoteOn = true;
        }
    }
}

void AnalogToMidiSynthAudioProcessor::updateFilter()
{
    // gets the cutoff frequency and resonance parameters from the value tree
    float cutoff = apvts.getRawParameterValue("cutoff")->load();
    float resonance = apvts.getRawParameterValue("resonance")->load();
    
    // setting the filter as a lowpass and giving it the cutoff frequency and resonance
    lpf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
    lpf.setCutoffFrequency(cutoff);
    lpf.setResonance(resonance);
}

// this gets the parameter values from apvts
// and sets the adsr object parameters
void AnalogToMidiSynthAudioProcessor::updateADSR()
{
    // gets the adsr paramaters from the value tree
    float attack = apvts.getRawParameterValue("attack")->load();
    float decay = apvts.getRawParameterValue("decay")->load();
    float sustain = apvts.getRawParameterValue("sustain")->load();
    float release = apvts.getRawParameterValue("release")->load();
    
    juce::ADSR::Parameters adsrParams;
    
    adsrParams.attack = attack;
    adsrParams.decay = decay;
    adsrParams.sustain = sustain;
    adsrParams.release = release;
    
    adsr.setParameters(adsrParams);
}

void AnalogToMidiSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AnalogToMidiSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AnalogToMidiSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // checking if the flags are correctly handled to prevent thread races
    if (triggerNoteOn.exchange(false)) {
        adsr.noteOn();
    }
    if (triggerNoteOff.exchange(false)) {
        adsr.noteOff();
    }
    
    int waveform = apvts.getRawParameterValue("waveform")->load();
    
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    

    // calculating the phase delta so that we can use it for the waveform generation
    float currentFreq = targetFrequency.load();
    level = targetLevel.load();
    phaseDelta = (float) (currentFreq / currentSampleRate);
    
    
    // writing pointers
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    

    if (waveform == 0) { // sine wave
        for ( int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            
            // calc the sin wave
            float currentSample = std::sin(phase * juce::MathConstants<float>::twoPi) * level;
            
            // advancing the phase
            phase = std::fmod ( phase + phaseDelta, 1.0f);
            
            // write the sample to the audio buffers
            leftChannel[ sample ] = currentSample;
            
            if ( buffer.getNumChannels() > 1) {
                rightChannel[ sample ] = currentSample;
            }
        }
    }
    else if (waveform == 1) { // sawtooth
        for ( int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            // calculating the sawtooth wave
            float currentSample = ((phase * 2.0f) - 1.0f) * level;
    
            // advancing the phase
            phase = std::fmod ( phase + phaseDelta, 1.0f);
            
            // write the sample to the audio buffers
            leftChannel[ sample ] = currentSample;
            
            if ( buffer.getNumChannels() > 1) {
                rightChannel[ sample ] = currentSample;
            }
        }
    }
    else if (waveform == 2) { // triangle
        for ( int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            // calcualting the traingle wave
            float currentSample = (1.0f - 4.0f * std::fabs(phase - 0.5f)) * level;
            
            // advancing the phase
            phase = std::fmod ( phase + phaseDelta, 1.0f);
            
            // write the sample to the audio buffers
            leftChannel[ sample ] = currentSample;
            
            if ( buffer.getNumChannels() > 1) {
                rightChannel[ sample ] = currentSample;
            }
        }
    }
    
    // applying the adsr
    updateADSR();
    adsr.applyEnvelopeToBuffer(buffer, 0, buffer.getNumSamples());
    
    // this applies the low pass filter with specified resonance and cutoff freq
    updateFilter();
    juce::dsp::AudioBlock<float> block(buffer);
    lpf.process(juce::dsp::ProcessContextReplacing<float>(block));
    
}

//==============================================================================
bool AnalogToMidiSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AnalogToMidiSynthAudioProcessor::createEditor()
{
    return new AnalogToMidiSynthAudioProcessorEditor (*this);
}

//==============================================================================
void AnalogToMidiSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AnalogToMidiSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogToMidiSynthAudioProcessor();
}
