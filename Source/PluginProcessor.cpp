/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SchroederVerbAudioProcessor::SchroederVerbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    mFBCFRingBuf.debug(true);
    mERDelayRingBuf.debug(true);
}

SchroederVerbAudioProcessor::~SchroederVerbAudioProcessor()
{
}

//==============================================================================
void SchroederVerbAudioProcessor::clearAllBuffers()
{
    mFBCFRingBuf.init();
    mERDelayRingBuf.init();
    mMMBuf.clear();
    mERBuf.clear();
    mDelayBlockBuf.clear();
}
void SchroederVerbAudioProcessor::setMMBufValue(int channel, double bufferIndex)
{
    channel=(channel > 0) ? 0 :channel;
    channel=(channel < 1) ? 1 :channel;
    
    bufferIndex = (bufferIndex < 0) ? 0 : bufferIndex;
    bufferIndex = (bufferIndex > 3) ? 3 : bufferIndex;
    
    
    if (channel==0) //how do I do this with enums?
        mMMOutLeft = bufferIndex;
    else
        mMMOUtRight = bufferIndex;
    
}
int SchroederVerbAudioProcessor::getMMBufValue(int channel)
{
    if (channel == 0)
        return mMMOutLeft;
    else
        return mMMOUtRight;
}
double SchroederVerbAudioProcessor::getERDelayMs(int channel)
{
    return mERDelaysMs[channel];
    
}

void SchroederVerbAudioProcessor::setERDelayMs(int index, double value)
{
   // check index is betwen 0 and num-1
    mERDelaysMs[index] = value;
    // convert value to seconds
    
    mERDelSamps[index] = juce::roundToInt(atec::Utilities::sec2samp(value / 100.0, mSampleRate));
    
    DBG("ERDelSamps: " + juce::String(mERDelSamps[index]));
    
}

double SchroederVerbAudioProcessor::getFilterDelay(int channel)
{
    return mCombFilterDelaysMs[channel];
}

void SchroederVerbAudioProcessor::setFilterDelay(int index, double value)
{
    mCombFilterDelaysMs[index] = value;
    mCombFilterDelaysMs[index] = juce::roundToInt(atec::Utilities::sec2samp(value / 100.0, mSampleRate));
    
}

double SchroederVerbAudioProcessor::getFilterFeedback(int channel)
{
    return mCombFilterFeedbacks[channel];
}

void SchroederVerbAudioProcessor::setFilterFeedback(int index, double value)
{
    mCombFilterFeedbacks[index] = value;
    mCombFilterFeedbacks[index] = juce::roundToInt(atec::Utilities::sec2samp(value / 100.0, mSampleRate));
}

                                                

void SchroederVerbAudioProcessor::doEarlyReflections(juce::AudioBuffer<float>& buffer)
{
    // create early reflection simulation by mixing/delaying the left/right channels
    int bufSize = buffer.getNumSamples();

    // start by clearing the ER buf, our workspace for the stereo all-pass signals
    mERBuf.clear();

    // make a for loop for each stage of early reflections
    for (int stage = 0; stage < NUMER; ++stage)
    {
        // the buffer argument to processBlock(), 2 channels and <= mBlockSize samples per channel
        auto* lBufReadPtr = buffer.getReadPointer(0);
        auto* rBufReadPtr = buffer.getReadPointer(1);
        // mERBuf has 2 channels and mBlockSize samples per channel
        auto* lErBufPtr = mERBuf.getReadPointer(0);
        auto* rErBufPtr = mERBuf.getWritePointer(1);

        // left channel needs a special case if/else
        // for the first stage, copy from the input buffer
        if (stage == 0)
        {
            // mix L+R inputs into channel 0 of ER buf
            mERBuf.copyFrom (0, 0, buffer, 0, 0, bufSize);
            mERBuf.addFrom (0, 0, buffer, 1, 0, bufSize);
            mERBuf.applyGain (0, 0, bufSize, APGAIN); // reduce gain

            // mix L-R inputs into this stage of mERDelayBuf
            // we'll write to the mERDelayBuf RingBuffer one sample at a time so we can calculate the (L-R) * APGAIN value
            for (int sample = 0; sample < bufSize; ++sample)
                mERDelayRingBuf.writeSample (stage, sample, (lBufReadPtr[sample] - rBufReadPtr[sample]) * APGAIN);
        }
        else // for subsequent stages, accumulate
        {
            // mix channel 1 with channel 0 of ER buf
            mERBuf.addFrom (0, 0, mERBuf, 1, 0, bufSize);
            mERBuf.applyGain (0, 0, bufSize, APGAIN); // reduce gain

            // mix L-R inputs into ER delay buf for this channel
            // we'll write to the mERDelayBuf RingBuffer one sample at a time so we can calculate the (L-R) * APGAIN value
            for (int sample = 0; sample < bufSize; ++sample)
                mERDelayRingBuf.writeSample (stage, sample, (lErBufPtr[sample] - rErBufPtr[sample]) * APGAIN);
        }

        // for right channel, pull a block of delayed audio from mERDelayBuf into channel 1 of mERBuf
        // delay by mERDelSamps[stage] for this stage
        // we could use the .readUnsafe() method to get lower latency
        mERDelayRingBuf.read (stage, mERDelSamps[stage], mERBuf, 1, bufSize);
    }
}

void SchroederVerbAudioProcessor::doFeedbackCombFilters(int bufSize)
{
    for (int channel = 0; channel < NUMFBCF; ++channel)
    {
        // pull bufSize samples from the RingBuffer with a delay determined by FBCF channel
        mFBCFRingBuf.read (channel, mFBCFDelSamps[channel], mDelayBlockBuf, channel, bufSize);

        // reduce amplitude of the delayed block
        mDelayBlockBuf.applyGain (channel, 0, bufSize, mFBCFFdbkCoeffs[channel]);

        // alternate ER channels to pull from, adding to the amp-reduced delayed audio block.
        // there are 2 ER buffer channels, so put channel 0 into channels 0 and 2 of mDelayBlockBuf
        // and channel 1 into channels 1 and 3. that way both ER buffer channels are used equally
        mDelayBlockBuf.addFrom (channel, 0, mERBuf, channel % 2, 0, bufSize);

        // throw the mix back into the delay buffer at the current write position. we pass a 'false' to the advance write index argument of RingBuffer's write() method because we're going to advance the write index manually later
        mFBCFRingBuf.write (channel, mDelayBlockBuf, channel, bufSize, false);
    }
}

void SchroederVerbAudioProcessor::doMixingMatrix(int bufSize)
{
    // start with a clean buffer of silence
    mMMBuf.clear();

    // OutA: s1+s2
    mMMBuf.copyFrom (0, 0, mDelayBlockBuf, 0, 0, bufSize);
    mMMBuf.addFrom (0, 0, mDelayBlockBuf, 2, 0, bufSize);
    mMMBuf.addFrom (0, 0, mDelayBlockBuf, 1, 0, bufSize);
    mMMBuf.addFrom (0, 0, mDelayBlockBuf, 3, 0, bufSize);

    // OutB: negation of OutA
    mMMBuf.copyFrom (1, 0, mMMBuf, 0, 0, bufSize);
    mMMBuf.applyGain (1, 0, bufSize, -1.0);

    // OutD: s1 mix
    mMMBuf.copyFrom (3, 0, mDelayBlockBuf, 0, 0, bufSize);
    mMMBuf.addFrom (3, 0, mDelayBlockBuf, 2, 0, bufSize);

    // OutD: s2 mix, temporarily stored in mMMBuf channel 2 to mix and negate, then add into channel 3 along with s1 which is already there
    mMMBuf.copyFrom (2, 0, mDelayBlockBuf, 1, 0, bufSize);
    mMMBuf.addFrom (2, 0, mDelayBlockBuf, 3, 0, bufSize);
    mMMBuf.applyGain (2, 0, bufSize, -1.0);
    mMMBuf.addFrom (3, 0, mMMBuf, 2, 0, bufSize);

    // OutC: negation of OutD
    mMMBuf.copyFrom (2, 0, mMMBuf, 3, 0, bufSize);
    mMMBuf.applyGain (2, 0, bufSize, -1.0);
}

//==============================================================================
const juce::String SchroederVerbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SchroederVerbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SchroederVerbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SchroederVerbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SchroederVerbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SchroederVerbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SchroederVerbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SchroederVerbAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SchroederVerbAudioProcessor::getProgramName (int index)
{
    return {};
}

void SchroederVerbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void SchroederVerbAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    mBlockSize = samplesPerBlock;

    // we have NUMFBCF feedback comb filters that need to be combined with direct input
    // let's make these buffers quite long in duration so we can have long tails
    mFBCFRingBuf.setSize (NUMFBCF, 10.0 * mSampleRate, mBlockSize);
    // these are for the delays on the right channel of the early reflection chain
    // they should be the same duration as the ring buffer
    mERDelayRingBuf.setSize (NUMER, mFBCFRingBuf.getSize(), mBlockSize);

    // we need space to mix the L/R early reflection channels, so 2 channels total
    mERBuf.setSize (2, mBlockSize);
    // finally, we need space to mix one block of samples in the mixing matrix
    mMMBuf.setSize (NUMFBCF, mBlockSize);

    // this is temporary storage for writing the signal of each channel before final output
    mDelayBlockBuf.setSize (NUMFBCF, mBlockSize);

    // clear all the buffers to start
    clearAllBuffers();

    // from: https://ccrma.stanford.edu/~jos/pasp/Schroeder_Reverberators.html, figure A
    for (int erStage = 0; erStage < NUMER; ++erStage)
        mERDelSamps[erStage] = juce::roundToInt(atec::Utilities::sec2samp (mERDelaysMs[erStage] / 1000.0, mSampleRate));

    for (int channel = 0; channel < NUMFBCF; ++channel)
    {
        // these could be doubles if we interpolate, but there's not a need right now
        mFBCFDelSamps[channel] = juce::roundToInt(atec::Utilities::sec2samp (mCombFilterDelaysMs[channel] / 1000.0, mSampleRate));
        mFBCFFdbkCoeffs[channel] = mCombFilterFeedbacks[channel] / 100.0;
    }
    
    setMMBufValue(0,0);
    setMMBufValue(1,3);
    

    DBG("ERDelSamps: [" + juce::String (mERDelSamps[0]) + ", " + juce::String (mERDelSamps[1]) + ", " + juce::String (mERDelSamps[2]) + "]");
}

void SchroederVerbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SchroederVerbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void SchroederVerbAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    int bufSize = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (totalNumInputChannels>1)
    {
    // do early reflections
    doEarlyReflections(buffer);

    // do feedback comb filters
    doFeedbackCombFilters(bufSize);

    // now do the mixing matrix DSP
    doMixingMatrix(bufSize);

    // tap OutA and OutC for the stereo channels we send back to the host
    
        buffer.copyFrom (0, 0, mMMBuf, mMMOutLeft, 0, bufSize);
        buffer.copyFrom (1, 0, mMMBuf, mMMOUtRight, 0, bufSize);
    }
    
    // uncomment these lines to hear what it sounds like to bypass the mixing matrix stage
//    buffer.copyFrom (0, 0, mDelayBlockBuf, 0, 0, bufSize);
//    buffer.copyFrom (1, 0, mDelayBlockBuf, 2, 0, bufSize);
    
    // reduce to 40% gain on all output channels. this could be adjustable
    buffer.applyGain(0, bufSize, 0.4);

    // we need to advance both of the RingBuffer write indices manually here
    mFBCFRingBuf.advanceWriteIdx(bufSize);
    mERDelayRingBuf.advanceWriteIdx(bufSize);
}

//==============================================================================
bool SchroederVerbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SchroederVerbAudioProcessor::createEditor()
{
    return new SchroederVerbAudioProcessorEditor (*this);
}

//==============================================================================
void SchroederVerbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void SchroederVerbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SchroederVerbAudioProcessor();
}
