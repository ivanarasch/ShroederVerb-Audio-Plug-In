/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#define NUMFBCF 4
#define NUMER 5
#define APGAIN 0.7

//==============================================================================
/**
*/
class SchroederVerbAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SchroederVerbAudioProcessor();
    ~SchroederVerbAudioProcessor() override;

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

    double mSampleRate;
    double mBlockSize;
    
    double getERDelayMs (int index);
    void setERDelayMs (int index, double value);
    
    double getFilterDelay (int index);
    void setFilterDelay (int index, double value);
    
    double getFilterFeedback (int index);
    void setFilterFeedback(int index, double value);
    
    void clearAllBuffers();
    
    int matrixNumberL;
    int matrixNumberR;
    
    
    void setMMBuffValue (int channel, double value);
    
    

    double mAPGainliderValue;
    
    int getMMBufValue(int index);
    void setMMBufValue (int channel, double bufferIndex);
    

private:
    atec::RingBuffer mFBCFRingBuf;
    atec::RingBuffer mERDelayRingBuf;
    juce::AudioBuffer<float> mMMBuf;
    juce::AudioBuffer<float> mERBuf; // our workspace for processing the [AP] signals
    juce::AudioBuffer<float> mDelayBlockBuf;
    
    // using early reflection, feedback comb filter delay times, and feedback gains as suggested in https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html
    double mERDelaysMs[NUMER] = {28.31, 19.82, 13.88, 4.52, 1.48};
    double mCombFilterDelaysMs[NUMFBCF] = {67.48, 64.04, 82.12, 90.04};
    double mCombFilterFeedbacks[NUMFBCF] = {77.3, 80.2, 75.3, 73.3};
    
    // to convert ms to samples: round(msVal / 1000.0) * mSampleRate
    // could also use atec::Utilities::sec2samp(timeInSec, mSampleRate)
    
    int mMMOutLeft;
    int mMMOUtRight;
    
    int mERDelSamps[NUMER];
    int mFBCFDelSamps[NUMFBCF];
    float mFBCFFdbkCoeffs[NUMFBCF];

    void doEarlyReflections(juce::AudioBuffer<float>& buffer);
    void doFeedbackCombFilters(int bufSize);
    void doMixingMatrix(int bufSize);
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SchroederVerbAudioProcessor)
};
