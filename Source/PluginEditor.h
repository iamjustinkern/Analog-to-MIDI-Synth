/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class AnalogToMidiSynthAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    AnalogToMidiSynthAudioProcessorEditor (AnalogToMidiSynthAudioProcessor&);
    ~AnalogToMidiSynthAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    


private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AnalogToMidiSynthAudioProcessor& audioProcessor;
    
    // for the waveform selection
    juce::ComboBox waveformMenu;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    
    // ADSR Sliders
    juce::Slider attackSlider;
    juce::Label attackLabel;
    
    juce::Slider decaySlider;
    juce::Label decayLabel;
    
    juce::Slider sustainSlider;
    juce::Label sustainLabel;
    
    juce::Slider releaseSlider;
    juce::Label releaseLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    
    // cuttoff and resonance control
    juce::Slider cutoffSlider;
    juce::Label cutoffLabel;
    
    juce::Slider resonanceSlider;
    juce::Label resonanceLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> cutoffAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogToMidiSynthAudioProcessorEditor)
};
