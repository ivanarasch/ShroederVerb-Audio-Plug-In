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
class SchroederVerbAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Slider::Listener, public juce::ComboBox::Listener, public juce::ToggleButton::Listener
{
public:
    SchroederVerbAudioProcessorEditor (SchroederVerbAudioProcessor&);
    ~SchroederVerbAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SchroederVerbAudioProcessor& audioProcessor;
    
    juce::Slider mERDelayTimeSliderArray[NUMER];
    
    juce::Slider mAPGainSlider;
    
    juce::Slider mFilterDelayTimesSlider[NUMFBCF];
    juce::Slider mFilterFeedbackGainSlider[NUMFBCF];
    
    
    juce::ComboBox mMixingMatrixComboxL;
    juce::ComboBox mMixingMatrixComboxR;
    
    juce::ToggleButton mClearButton;
    
    
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* toggleButton) override;
    
    
    
    enum MixMatrixOut
    {
        outA = 1,
        outB = 2,
        outC = 3,
        outD = 4,
    };
   
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchroederVerbAudioProcessorEditor)
};
