/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SchroederVerbAudioProcessorEditor::SchroederVerbAudioProcessorEditor (SchroederVerbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    
    
     //Make sure that before the constructor has finished, you've set the
     //editor's size to whatever you need it to be.
    setSize (700, 500);
    
    
    for (int i=0; i<NUMER; i++)
    {
        mERDelayTimeSliderArray[i].setSliderStyle(juce::Slider::LinearVertical);
        mERDelayTimeSliderArray[i].setRange(0.0,100.0,0.01);
        mERDelayTimeSliderArray[i].setValue(audioProcessor.getERDelayMs(i));
        addAndMakeVisible(&mERDelayTimeSliderArray[i]);
        mERDelayTimeSliderArray[i].addListener(this);
        mERDelayTimeSliderArray[i].setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 100, 50);
    }

    for (int i=0; i<NUMFBCF; i++)
    {
        mFilterDelayTimesSlider[i].setSliderStyle(juce::Slider::LinearVertical);
        mFilterDelayTimesSlider[i].setRange(0.0,50.0,0.01);
        mFilterDelayTimesSlider[i].setValue(audioProcessor.getFilterDelay(i));
        addAndMakeVisible(&mFilterDelayTimesSlider[i]);
        mFilterDelayTimesSlider[i].addListener(this);
        mFilterDelayTimesSlider[i].setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 100, 50);
        
    }
    
    for (int i=0; i<NUMFBCF; i++)
    {
        mFilterFeedbackGainSlider[i].setSliderStyle(juce::Slider::LinearVertical);
        mFilterFeedbackGainSlider[i].setRange(0.0,100.0,0.01);
        mFilterFeedbackGainSlider[i].setValue(audioProcessor.getFilterFeedback(i));
        addAndMakeVisible(&mFilterFeedbackGainSlider[i]);
        mFilterFeedbackGainSlider[i].addListener(this);
        mFilterFeedbackGainSlider[i].setTextBoxStyle(juce::Slider::TextEntryBoxPosition::TextBoxBelow, true, 100, 50);

    }
    
    mAPGainSlider.setRange(0.0,1.0,0.1);
    mAPGainSlider.getValue();
    addAndMakeVisible(&mAPGainSlider);
    mAPGainSlider.addListener(this);
    


    
    mMixingMatrixComboxL.addItem("AL", outA);
    mMixingMatrixComboxL.addItem("BL", outB);
    mMixingMatrixComboxL.addItem("CL", outC);
    mMixingMatrixComboxL.addItem("DL", outD);
    mMixingMatrixComboxL.setSelectedId(audioProcessor.getMMBufValue(outA));
    addAndMakeVisible(&mMixingMatrixComboxL);
    mMixingMatrixComboxL.addListener(this);
    
    
    mMixingMatrixComboxR.addItem("AR", outA);
    mMixingMatrixComboxR.addItem("BR", outB);
    mMixingMatrixComboxR.addItem("CR", outC);
    mMixingMatrixComboxR.addItem("DR", outD);
    mMixingMatrixComboxR.setSelectedId(audioProcessor.getMMBufValue(outC));
    addAndMakeVisible(&mMixingMatrixComboxR);
    mMixingMatrixComboxR.addListener(this);
    
    
    addAndMakeVisible(&mClearButton);
    mClearButton.addListener(this);
    
    
}


SchroederVerbAudioProcessorEditor::~SchroederVerbAudioProcessorEditor()
{
    for(int i=0; i<NUMER; i++)
    mERDelayTimeSliderArray[i].removeListener(this);
    
    for(int i=0; i<NUMFBCF; i++)
    {
        mFilterDelayTimesSlider[i].removeListener(this);
    }
    mAPGainSlider.removeListener(this);
    
    mMixingMatrixComboxR.removeListener(this);
    mMixingMatrixComboxL.removeListener(this);
    
}

//==============================================================================
void SchroederVerbAudioProcessorEditor::sliderValueChanged(juce::Slider* slider)
{
    
    for (int i=0; i<NUMER; i++)
    {
        if (slider == &mERDelayTimeSliderArray[i])
        {
            audioProcessor.setERDelayMs(i, mERDelayTimeSliderArray[i].getValue());
        }
    }
    for (int i=0; i<NUMFBCF; i++)
    {
        if (slider == &mFilterDelayTimesSlider[i])
            {
                audioProcessor.setFilterDelay(i, mFilterDelayTimesSlider[i].getValue());
            }

        if (slider == &mFilterFeedbackGainSlider[i])
        {
            audioProcessor.setFilterFeedback(i, mFilterFeedbackGainSlider[i].getValue() / 100.0);

        }

    }
    if (slider == &mAPGainSlider)
    {
        audioProcessor.mAPGainliderValue = mAPGainSlider.getValue();
        
    }

}

void SchroederVerbAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBox)
{
    
    if( comboBox==  &mMixingMatrixComboxL)
        switch (mMixingMatrixComboxL.getSelectedId())
    {
        case outA:
            audioProcessor.setMMBufValue(0,0);
            break;
        case outB:
            audioProcessor.setMMBufValue(0,1);
            break;
        case outC:
            audioProcessor.setMMBufValue(0,2);
            break;
        case outD:
            audioProcessor.setMMBufValue(0,3);
            break;
        
        default:
            break;
    }

    else if( comboBox == &mMixingMatrixComboxL)
        switch (mMixingMatrixComboxR.getSelectedId())
    {
        case outA:
            audioProcessor.setMMBufValue(1,0);
            break;
        case outB:
            audioProcessor.setMMBufValue(1,1);
            break;
        case outC:
            audioProcessor.setMMBufValue(1,2);
            break;
        case outD:
            audioProcessor.setMMBufValue(1,3);
            break;
        
        default:
            break;
    }
}

void SchroederVerbAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button ==&mClearButton)
    {
        audioProcessor.clearAllBuffers();
    }
}



void SchroederVerbAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::hotpink);
}

void SchroederVerbAudioProcessorEditor::resized()
{
    for (int slider=0; slider<NUMER; slider++)
    mERDelayTimeSliderArray[slider].setBounds((50 * slider)+15,50,50,200);
    
    for (int slider=0; slider<NUMFBCF ; slider++)
    mFilterDelayTimesSlider[slider].setBounds((50 * slider)+15,300,50,200);
    
    for(int slider=0; slider<NUMFBCF; slider++)
    mFilterFeedbackGainSlider[slider].setBounds(350+(50 * slider)+15,50,50,200);
    
    mAPGainSlider.setBounds(350,270,300,100);
    mMixingMatrixComboxL.setBounds(350, 350, 70, 70);
    mMixingMatrixComboxR.setBounds(450, 350, 70, 70);
    
    mClearButton.setBounds(530, 310, 200,200);
    
    
    
}
