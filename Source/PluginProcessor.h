/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class AnalogToMidiSynthAudioProcessor  : public juce::AudioProcessor, private juce::OSCReceiver, private juce::OSCReceiver::ListenerWithOSCAddress<juce::OSCReceiver::MessageLoopCallback>
{
public:
    //==============================================================================
    AnalogToMidiSynthAudioProcessor();
    ~AnalogToMidiSynthAudioProcessor() override;
    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    

    
    //param layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    // state manager
    juce::AudioProcessorValueTreeState apvts;
    
    void updateFilter();
    void updateADSR();

private:
    double currentSampleRate = 44100.0;
    
    // modulo counter var
    float phase = 0.0f;  // tracks progress 0.0 to 1.0
    float phaseDelta = 0.0f; // how much the phase advances per sample
    float level = 0.0f;  // amplitude (volume)
    
    int waveform = 0; // default sine (1) sawtooth (2) triangle (3)
    
    // filter object
    juce::dsp::StateVariableTPTFilter<float> lpf;

    
    // adsr object
    juce::ADSR adsr;
    
    
    // Thread-safe variables for our OSC bridge
    std::atomic<float> targetFrequency { 440.0f };
    std::atomic<float> targetLevel { 0.0f };
    
    // More thread saftey for the OSC messages
    std::atomic<bool> triggerNoteOn { false };
    std::atomic<bool> triggerNoteOff { false };
    
    
    // osc
    void oscMessageReceived( const juce::OSCMessage& message) override;
    void showConnectionErrorMessage( const juce::String& messageText);
    juce::ScopedMessageBox messageBox;

    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogToMidiSynthAudioProcessor)
};
