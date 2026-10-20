#pragma once

#include "PluginProcessor.h"

//==============================================================================
class ShapetratorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ShapetratorLookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};

//==============================================================================
/** Egy sáv 16 rajzolható lépéssel. Kattintás/húzás = érték beállítása, dupla kattintás = alapérték. */
class LaneEditor : public juce::Component
{
public:
    LaneEditor (ShapetratorProcessor& p, int laneIndex, juce::Colour colour, bool bipolarLane);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    int stepAt (float x) const;
    void handleMouse (juce::Point<float> position);
    void setStep (int step, float value);

    ShapetratorProcessor& proc;
    const int lane;
    const juce::Colour colour;
    const bool bipolar;

    juce::RangedAudioParameter* params[ShapetratorProcessor::maxSteps] {};
    int lastStep = -1;
    float lastValue = 0.0f;
};

//==============================================================================
class ShapetratorEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit ShapetratorEditor (ShapetratorProcessor&);
    ~ShapetratorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Combo
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& text, const juce::String& paramId,
                    juce::Colour colour, bool percent);
    void setupCombo (Combo&, const juce::String& text, const juce::String& paramId);

    ShapetratorProcessor& proc;
    ShapetratorLookAndFeel lnf;

    juce::TextButton randomButton { "RANDOM" };
    Combo rateBox, ftypeBox;
    Knob stepsKnob, smoothKnob, resoKnob, mixKnob;
    Knob depthKnob[ShapetratorProcessor::numLanes];
    std::unique_ptr<LaneEditor> lanes[ShapetratorProcessor::numLanes];

    juce::Rectangle<int> headerArea, globalsArea, laneRow[ShapetratorProcessor::numLanes];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShapetratorEditor)
};
