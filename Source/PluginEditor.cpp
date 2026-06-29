/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogToMidiSynthAudioProcessorEditor::AnalogToMidiSynthAudioProcessorEditor (AnalogToMidiSynthAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    

    
    setSize (400, 300);
    
    // cuttoff and resonance sliders
    cutoffSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    cutoffSlider.setRange(20.0f, 20000.0f);
    cutoffSlider.setValue(440.0f);
    cutoffSlider.setTextBoxStyle(juce::Slider::TextBoxBelow,false, 80, 20);
    cutoffSlider.setSkewFactor(1000.0f); // gives it the logarithmic skew
    addAndMakeVisible(cutoffSlider);
    cutoffLabel.setText("Cutoff Freq", juce::dontSendNotification);
    cutoffLabel.setJustificationType(juce::Justification::centred);
    cutoffLabel.attachToComponent(&cutoffSlider, false);
    addAndMakeVisible(cutoffLabel);
    cutoffAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.apvts, "cutoff", cutoffSlider);
    
    resonanceSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    resonanceSlider.setRange(0.1f, 10.0f);
    resonanceSlider.setValue(2.0f);
    resonanceSlider.setTextBoxStyle(juce::Slider::TextBoxBelow,false, 50, 20);
    addAndMakeVisible(resonanceSlider);
    resonanceLabel.setText("Resonance", juce::dontSendNotification);
    resonanceLabel.setJustificationType(juce::Justification::centred);
    resonanceLabel.attachToComponent(&resonanceSlider, false);
    addAndMakeVisible(resonanceLabel);
    resonanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.apvts, "resonance", resonanceSlider);
    
    // waveform menu
    waveformMenu.addItem("Sine", 1);
    waveformMenu.addItem("Sawtooth", 2);
    waveformMenu.addItem("Triangle", 3);
    addAndMakeVisible(waveformMenu);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "waveform", waveformMenu);
    
    // ADSR Sliders
    attackSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    attackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(attackSlider);
    attackLabel.setText("Attack", juce::dontSendNotification);
    attackLabel.setJustificationType(juce::Justification::centred);
    attackLabel.attachToComponent(&attackSlider, false);
    addAndMakeVisible(attackLabel);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "attack", attackSlider);
    
    decaySlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    decaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(decaySlider);
    decayLabel.setText("Decay", juce::dontSendNotification);
    decayLabel.setJustificationType(juce::Justification::centred);
    decayLabel.attachToComponent(&decaySlider, false);
    addAndMakeVisible(decayLabel);
    decayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "decay", decaySlider);
    
    sustainSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    sustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(sustainSlider);
    sustainLabel.setText("Sustain", juce::dontSendNotification);
    sustainLabel.setJustificationType(juce::Justification::centred);
    sustainLabel.attachToComponent(&sustainSlider, false);
    addAndMakeVisible(sustainLabel);
    sustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "sustain", sustainSlider);
    
    releaseSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    addAndMakeVisible(releaseSlider);
    releaseLabel.setText("Release", juce::dontSendNotification);
    releaseLabel.setJustificationType(juce::Justification::centred);
    releaseLabel.attachToComponent(&releaseSlider, false);
    addAndMakeVisible(releaseLabel);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "release", releaseSlider);
    
    
}



AnalogToMidiSynthAudioProcessorEditor::~AnalogToMidiSynthAudioProcessorEditor()
{
}

//==============================================================================
void AnalogToMidiSynthAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));


}

void AnalogToMidiSynthAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    // splitting the screen in half
    auto topHalf = area.removeFromTop(area.getHeight() / 2);
    auto bottomHalf = area;
    
    // adding more room to my UI so that all the labels show
    topHalf.removeFromTop(20);
    bottomHalf.removeFromTop(20);
    
    // resonance and cutoff and the waveform menu
    int topColumns = 3;
    cutoffSlider.setBounds(topHalf.removeFromLeft(topHalf.getWidth() / topColumns));
    resonanceSlider.setBounds(topHalf.removeFromLeft(topHalf.getWidth() / (topColumns)));
    waveformMenu.setBounds(topHalf.withSizeKeepingCentre(100, 30));
    
    // the adsr sliders go below the rest
    int belowColumns = 4;
    attackSlider.setBounds(bottomHalf.removeFromLeft(bottomHalf.getWidth() / belowColumns));
    decaySlider.setBounds(bottomHalf.removeFromLeft(bottomHalf.getWidth() / (belowColumns)));
    sustainSlider.setBounds(bottomHalf.removeFromLeft(bottomHalf.getWidth() / (belowColumns)));
    releaseSlider.setBounds(bottomHalf);
}
