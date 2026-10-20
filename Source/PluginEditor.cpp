#include "PluginEditor.h"

namespace
{
    const juce::Colour colBackground { 0xff12141a };
    const juce::Colour colPanel      { 0xff1b1e27 };
    const juce::Colour colText       { 0xffaab0c0 };
    const juce::Colour colAccent     { 0xffb06bff };

    juce::Colour laneColour (int lane)
    {
        static const juce::Colour colours[] = { juce::Colour (0xff2ee6a6),   // Volume - menta
                                                juce::Colour (0xff4aa8ff),   // Filter - kék
                                                juce::Colour (0xffffb347),   // Pan    - narancs
                                                juce::Colour (0xffff4d7a) }; // Crush  - rózsaszín
        return colours[lane];
    }

    juce::Font makeFont (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }
}

//==============================================================================
// LookAndFeel
//==============================================================================
ShapetratorLookAndFeel::ShapetratorLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, colText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);

    setColour (juce::Label::textColourId, colText);

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff232733));
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff343a4a));
    setColour (juce::ComboBox::arrowColourId, colText);

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1e27));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, colAccent);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::TextButton::buttonColourId, colAccent);
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void ShapetratorLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPos, float startAngle, float endAngle,
                                               juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (4.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre  = bounds.getCentre();
    const float arcR   = radius - 3.0f;
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);
    const auto stroke  = juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff2b2f3b));
    g.strokePath (track, stroke);

    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
    g.strokePath (value, stroke);

    const float bodyR = arcR - 7.0f;
    g.setColour (juce::Colour (0xff262a35));
    g.fillEllipse (centre.x - bodyR, centre.y - bodyR, bodyR * 2.0f, bodyR * 2.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -bodyR + 2.0f, 3.0f, bodyR * 0.5f, 1.5f);
    g.setColour (juce::Colours::white);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void ShapetratorLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b,
                                                   const juce::Colour& backgroundColour,
                                                   bool highlighted, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto c = backgroundColour;

    if (down)             c = c.darker (0.25f);
    else if (highlighted) c = c.brighter (0.15f);

    g.setColour (c.withAlpha (0.25f));
    g.fillRoundedRectangle (r.expanded (1.5f), 10.0f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 9.0f);
}

juce::Font ShapetratorLookAndFeel::getTextButtonFont (juce::TextButton&, int)
{
    return makeFont (17.0f, true);
}

//==============================================================================
// LaneEditor
//==============================================================================
LaneEditor::LaneEditor (ShapetratorProcessor& p, int laneIndex, juce::Colour c, bool bipolarLane)
    : proc (p), lane (laneIndex), colour (c), bipolar (bipolarLane)
{
    for (int s = 0; s < ShapetratorProcessor::maxSteps; ++s)
        params[s] = proc.apvts.getParameter (ShapetratorProcessor::stepId (lane, s));

    setMouseCursor (juce::MouseCursor::CrosshairCursor);
}

int LaneEditor::stepAt (float x) const
{
    const float colW = (float) getWidth() / (float) ShapetratorProcessor::maxSteps;
    return juce::jlimit (0, ShapetratorProcessor::maxSteps - 1, (int) (x / colW));
}

void LaneEditor::setStep (int step, float value)
{
    if (auto* prm = params[step])
    {
        value = juce::jlimit (0.0f, 1.0f, value);

        if (std::abs (prm->getValue() - value) > 1.0e-4f)
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (value);
            prm->endChangeGesture();
        }
    }
}

void LaneEditor::handleMouse (juce::Point<float> p)
{
    const int step = stepAt (p.x);
    float v = juce::jlimit (0.0f, 1.0f, 1.0f - (p.y - 3.0f) / ((float) getHeight() - 6.0f));

    if (bipolar && std::abs (v - 0.5f) < 0.04f)  v = 0.5f;   // könnyű középre állítás
    if (! bipolar && v < 0.03f)                  v = 0.0f;   // könnyű nullázás

    if (lastStep < 0 || lastStep == step)
    {
        setStep (step, v);
    }
    else
    {
        // Gyors egérmozgásnál a kihagyott oszlopokat is kitöltjük
        const int dir = step > lastStep ? 1 : -1;

        for (int s = lastStep + dir; s != step + dir; s += dir)
        {
            const float t = (float) (s - lastStep) / (float) (step - lastStep);
            setStep (s, lastValue + t * (v - lastValue));
        }
    }

    lastStep = step;
    lastValue = v;
    repaint();
}

void LaneEditor::mouseDown (const juce::MouseEvent& e)
{
    lastStep = -1;
    handleMouse (e.position);
}

void LaneEditor::mouseDrag (const juce::MouseEvent& e)
{
    handleMouse (e.position);
}

void LaneEditor::mouseUp (const juce::MouseEvent&)
{
    lastStep = -1;
}

void LaneEditor::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int step = stepAt (e.position.x);

    if (auto* prm = params[step])
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->getDefaultValue());
        prm->endChangeGesture();
    }

    lastStep = -1;
    repaint();
}

void LaneEditor::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff14161d));
    g.fillRoundedRectangle (b, 6.0f);

    const int active  = proc.getActiveSteps();
    const int current = proc.getCurrentStep();
    const float colW  = b.getWidth() / (float) ShapetratorProcessor::maxSteps;

    for (int i = 0; i < ShapetratorProcessor::maxSteps; ++i)
    {
        const bool on = i < active;
        const bool playing = on && i == current;
        const auto col = juce::Rectangle<float> (b.getX() + (float) i * colW, b.getY(), colW, b.getHeight()).reduced (2.0f, 3.0f);

        // oszlop háttér (4-es csoportok enyhén eltérő árnyalattal)
        const float shade = ((i / 4) % 2 == 0) ? 0.0f : 0.03f;
        g.setColour (playing ? colour.withAlpha (0.16f)
                             : juce::Colour (0xff1b1f29).brighter (shade).withAlpha (on ? 1.0f : 0.35f));
        g.fillRoundedRectangle (col, 3.0f);

        // sáv
        const float v = proc.getStepValue (lane, i);
        const float y = col.getBottom() - v * col.getHeight();
        juce::Rectangle<float> bar;

        if (bipolar)
        {
            const float mid = col.getCentreY();
            bar = juce::Rectangle<float> (col.getX(), juce::jmin (mid, y), col.getWidth(), std::abs (mid - y));
        }
        else
        {
            bar = juce::Rectangle<float> (col.getX(), y, col.getWidth(), col.getBottom() - y);
        }

        g.setColour (colour.withAlpha (on ? (playing ? 1.0f : 0.78f) : 0.22f));
        g.fillRoundedRectangle (bar.withHeight (juce::jmax (bar.getHeight(), 2.0f)), 2.0f);
    }

    if (bipolar)
    {
        g.setColour (juce::Colours::white.withAlpha (0.12f));
        g.drawHorizontalLine ((int) b.getCentreY(), b.getX() + 2.0f, b.getRight() - 2.0f);
    }
}

//==============================================================================
// Editor
//==============================================================================
ShapetratorEditor::ShapetratorEditor (ShapetratorProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);

    // Globális vezérlők
    setupCombo (rateBox,  "RATE",   "rate");
    setupKnob  (stepsKnob,  "STEPS",  "steps",  colAccent, false);
    setupKnob  (smoothKnob, "SMOOTH", "smooth", colAccent, true);
    setupKnob  (resoKnob,   "RESO",   "reso",   laneColour (ShapetratorProcessor::Filter), true);
    setupCombo (ftypeBox, "FILTER", "ftype");
    setupKnob  (mixKnob,    "MIX",    "mix",    juce::Colours::white, true);

    // Sávok
    static const char* depthIds[] = { "depth_vol", "depth_flt", "depth_pan", "depth_crs" };

    for (int l = 0; l < ShapetratorProcessor::numLanes; ++l)
    {
        setupKnob (depthKnob[l], juce::String (ShapetratorProcessor::laneName (l)).toUpperCase(),
                   depthIds[l], laneColour (l), true);
        depthKnob[l].label.setColour (juce::Label::textColourId, laneColour (l));

        lanes[l] = std::make_unique<LaneEditor> (proc, l, laneColour (l), l == ShapetratorProcessor::Pan);
        addAndMakeVisible (*lanes[l]);
    }

    randomButton.onClick = [this] { proc.randomise(); };
    randomButton.setTooltip ("Véletlen preset");
    addAndMakeVisible (randomButton);

    setSize (820, 620);
    startTimerHz (30);
}

ShapetratorEditor::~ShapetratorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void ShapetratorEditor::setupKnob (Knob& k, const juce::String& text, const juce::String& paramId,
                                   juce::Colour colour, bool percent)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 16);
    k.slider.setColour (juce::Slider::rotarySliderFillColourId, colour);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                  juce::MathConstants<float>::pi * 2.8f, true);
    addAndMakeVisible (k.slider);

    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);

    if (percent)
    {
        k.slider.textFromValueFunction = [] (double v) { return juce::String (juce::roundToInt (v * 100.0)) + "%"; };
        k.slider.valueFromTextFunction = [] (const juce::String& t) { return t.getDoubleValue() / 100.0; };
        k.slider.updateText();
    }

    k.label.setText (text, juce::dontSendNotification);
    k.label.setJustificationType (juce::Justification::centred);
    k.label.setFont (makeFont (12.0f, true));
    addAndMakeVisible (k.label);
}

void ShapetratorEditor::setupCombo (Combo& c, const juce::String& text, const juce::String& paramId)
{
    if (auto* choice = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (paramId)))
        c.box.addItemList (choice->choices, 1);

    addAndMakeVisible (c.box);
    c.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramId, c.box);

    c.label.setText (text, juce::dontSendNotification);
    c.label.setJustificationType (juce::Justification::centred);
    c.label.setFont (makeFont (12.0f, true));
    addAndMakeVisible (c.label);
}

void ShapetratorEditor::timerCallback()
{
    for (auto& l : lanes)
        if (l != nullptr)
            l->repaint();
}

void ShapetratorEditor::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (colBackground.brighter (0.06f), 0.0f, 0.0f,
                                             colBackground, 0.0f, (float) getHeight(), false));
    g.fillAll();

    // Cím
    g.setColour (juce::Colours::white);
    g.setFont (makeFont (30.0f, true));
    g.drawText ("SHAPETRATOR", headerArea.withTrimmedRight (200), juce::Justification::centredLeft);

    g.setColour (colText.withAlpha (0.7f));
    g.setFont (makeFont (12.0f));
    g.drawText ("rajzolj mintát, formáld a hangot", headerArea.withTrimmedRight (200).withTrimmedLeft (230).translated (0, 8),
                juce::Justification::centredLeft);

    // Panelek
    g.setColour (colPanel);
    g.fillRoundedRectangle (globalsArea.toFloat(), 10.0f);

    for (int l = 0; l < ShapetratorProcessor::numLanes; ++l)
    {
        g.setColour (colPanel);
        g.fillRoundedRectangle (laneRow[l].toFloat(), 10.0f);
        g.setColour (laneColour (l));
        g.fillRoundedRectangle (laneRow[l].toFloat().withWidth (4.0f).reduced (0.0f, 10.0f), 2.0f);
    }
}

void ShapetratorEditor::resized()
{
    auto area = getLocalBounds().reduced (18);

    headerArea = area.removeFromTop (52);
    randomButton.setBounds (headerArea.removeFromRight (170).reduced (0, 6));

    area.removeFromTop (10);
    globalsArea = area.removeFromTop (100);

    auto placeKnob = [] (Knob& k, juce::Rectangle<int> r)
    {
        k.label.setBounds (r.removeFromTop (16));
        k.slider.setBounds (r);
    };

    auto placeCombo = [] (Combo& c, juce::Rectangle<int> r)
    {
        c.label.setBounds (r.removeFromTop (16));
        c.box.setBounds (r.withSizeKeepingCentre (juce::jmin (r.getWidth() - 16, 120), 28));
    };

    auto g = globalsArea.reduced (8, 8);
    const int colW = g.getWidth() / 6;

    placeCombo (rateBox,  g.removeFromLeft (colW));
    placeKnob  (stepsKnob,  g.removeFromLeft (colW));
    placeKnob  (smoothKnob, g.removeFromLeft (colW));
    placeKnob  (resoKnob,   g.removeFromLeft (colW));
    placeCombo (ftypeBox, g.removeFromLeft (colW));
    placeKnob  (mixKnob,    g);

    area.removeFromTop (10);
    const int gap = 8;
    const int rowH = (area.getHeight() - gap * (ShapetratorProcessor::numLanes - 1)) / ShapetratorProcessor::numLanes;

    for (int l = 0; l < ShapetratorProcessor::numLanes; ++l)
    {
        laneRow[l] = area.removeFromTop (rowH);
        area.removeFromTop (gap);

        auto r = laneRow[l].reduced (8, 8);
        r.removeFromLeft (6);
        placeKnob (depthKnob[l], r.removeFromLeft (92));
        r.removeFromLeft (6);
        lanes[l]->setBounds (r);
    }
}
