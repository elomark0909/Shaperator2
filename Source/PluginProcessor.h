#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

//==============================================================================
// Shapetrator - 4 sávos, tempóhoz szinkronizált "lépés-formázó" effekt.
// Minden sávhoz (Volume, Filter, Pan, Crush) 16 lépéses, rajzolható mintázat tartozik.
//==============================================================================
class ShapetratorProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numLanes = 4;
    static constexpr int maxSteps = 16;

    enum Lane { Volume = 0, Filter = 1, Pan = 2, Crush = 3 };

    ShapetratorProcessor();
    ~ShapetratorProcessor() override = default;

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Shapetrator"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==========================================================================
    /** Véletlen preset: új mintázatok, mélységek, ráta, simítás, szűrőtípus. A Mix-et nem bántja. */
    void randomise();

    static juce::String stepId (int lane, int step);
    static const char* laneName (int lane);

    int getCurrentStep() const noexcept { return currentStep.load(); }
    int getActiveSteps() const noexcept { return juce::jlimit (2, maxSteps, (int) std::lround (stepsRaw->load())); }
    float getStepValue (int lane, int step) const noexcept { return stepRaw[lane][step]->load(); }

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void updateFilter (float laneValue, float depth, int type, float reso) noexcept;
    float processFilter (int ch, float x, int type, float depth) noexcept;

    // Paraméter-mutatók (audio szálon biztonságosan olvashatók)
    std::atomic<float>* stepRaw[numLanes][maxSteps] {};
    std::atomic<float>* depthRaw[numLanes] {};
    std::atomic<float>* rateRaw   = nullptr;
    std::atomic<float>* stepsRaw  = nullptr;
    std::atomic<float>* smoothRaw = nullptr;
    std::atomic<float>* resoRaw   = nullptr;
    std::atomic<float>* ftypeRaw  = nullptr;
    std::atomic<float>* mixRaw    = nullptr;

    // DSP állapot
    double fs = 44100.0;
    double freeRunPos = 0.0;
    float laneSm[numLanes] {};
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> depthSm[numLanes];
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSm;

    float svfIc1[2] {}, svfIc2[2] {};
    float svfK = 1.0f, svfA1 = 0.0f, svfA2 = 0.0f, svfA3 = 0.0f;

    float crushHeld[2] {};
    float crushCounter = 0.0f;

    std::atomic<int> currentStep { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapetratorProcessor)
};
