#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>
#include <cmath>

namespace
{
    constexpr int numLanes = ShapetratorProcessor::numLanes;
    constexpr int maxSteps = ShapetratorProcessor::maxSteps;

    const char* const lanePrefix[numLanes] = { "vol", "flt", "pan", "crs" };
    const char* const laneNames[numLanes]  = { "Volume", "Filter", "Pan", "Crush" };

    constexpr int numRates = 7;
    const char* const rateNames[numRates]         = { "1/2", "1/4", "1/8", "1/8T", "1/16", "1/16T", "1/32" };
    constexpr double stepsPerBeatTable[numRates]  = { 0.5, 1.0, 2.0, 3.0, 4.0, 6.0, 8.0 };

    // Alapértelmezett mintázatok: már az első betöltéskor halljon valamit
    constexpr float defaultPattern[numLanes][maxSteps] =
    {
        { 1.00f, 0.25f, 0.70f, 0.25f,  1.00f, 0.25f, 0.70f, 0.25f,  1.00f, 0.25f, 0.70f, 0.25f,  1.00f, 0.25f, 0.70f, 0.50f }, // Volume
        { 0.00f, 0.07f, 0.13f, 0.20f,  0.27f, 0.33f, 0.40f, 0.47f,  0.53f, 0.60f, 0.67f, 0.73f,  0.80f, 0.87f, 0.93f, 1.00f }, // Filter
        { 0.10f, 0.90f, 0.10f, 0.90f,  0.10f, 0.90f, 0.10f, 0.90f,  0.10f, 0.90f, 0.10f, 0.90f,  0.10f, 0.90f, 0.10f, 0.90f }, // Pan
        { 0.00f, 0.00f, 0.00f, 0.00f,  0.00f, 0.00f, 0.00f, 0.80f,  0.00f, 0.00f, 0.00f, 0.00f,  0.00f, 0.60f, 0.90f, 1.00f }  // Crush
    };

    constexpr float defaultDepth[numLanes] = { 0.75f, 0.50f, 0.0f, 0.0f };

    const char* const depthIds[numLanes] = { "depth_vol", "depth_flt", "depth_pan", "depth_crs" };
}

//==============================================================================
juce::String ShapetratorProcessor::stepId (int lane, int step)
{
    return juce::String::formatted ("%s_%02d", lanePrefix[lane], step + 1);
}

const char* ShapetratorProcessor::laneName (int lane)
{
    return laneNames[lane];
}

juce::AudioProcessorValueTreeState::ParameterLayout ShapetratorProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    // Globális vezérlők
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Rate",
                                                        StringArray (rateNames, numRates), 4));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "steps", 1 }, "Steps", 2, maxSteps, maxSteps));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "smooth", 1 }, "Smooth",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.10f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "reso", 1 }, "Resonance",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.25f));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "ftype", 1 }, "Filter Type",
                                                        StringArray { "Lowpass", "Highpass", "Bandpass" }, 0));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f));

    // Sávonkénti mélység
    for (int l = 0; l < numLanes; ++l)
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { depthIds[l], 1 },
                                                           String (laneNames[l]) + " Depth",
                                                           NormalisableRange<float> (0.0f, 1.0f), defaultDepth[l]));

    // Lépés-értékek (sávonként 16)
    for (int l = 0; l < numLanes; ++l)
        for (int s = 0; s < maxSteps; ++s)
            layout.add (std::make_unique<AudioParameterFloat> (ParameterID { stepId (l, s), 1 },
                                                               String (laneNames[l]) + " Step " + String (s + 1),
                                                               NormalisableRange<float> (0.0f, 1.0f),
                                                               defaultPattern[l][s]));
    return layout;
}

//==============================================================================
ShapetratorProcessor::ShapetratorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int l = 0; l < numLanes; ++l)
    {
        depthRaw[l] = apvts.getRawParameterValue (depthIds[l]);

        for (int s = 0; s < maxSteps; ++s)
            stepRaw[l][s] = apvts.getRawParameterValue (stepId (l, s));
    }

    rateRaw   = apvts.getRawParameterValue ("rate");
    stepsRaw  = apvts.getRawParameterValue ("steps");
    smoothRaw = apvts.getRawParameterValue ("smooth");
    resoRaw   = apvts.getRawParameterValue ("reso");
    ftypeRaw  = apvts.getRawParameterValue ("ftype");
    mixRaw    = apvts.getRawParameterValue ("mix");
}

//==============================================================================
bool ShapetratorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();

    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void ShapetratorProcessor::prepareToPlay (double sampleRate, int)
{
    fs = sampleRate;

    for (int l = 0; l < numLanes; ++l)
    {
        depthSm[l].reset (fs, 0.02);
        depthSm[l].setCurrentAndTargetValue (depthRaw[l]->load());
        laneSm[l] = stepRaw[l][0]->load();
    }

    mixSm.reset (fs, 0.02);
    mixSm.setCurrentAndTargetValue (mixRaw->load());

    for (int ch = 0; ch < 2; ++ch)
    {
        svfIc1[ch] = svfIc2[ch] = 0.0f;
        crushHeld[ch] = 0.0f;
    }

    crushCounter = 0.0f;
    freeRunPos = 0.0;
    updateFilter (1.0f, 0.0f, 0, 0.0f);
}

//==============================================================================
void ShapetratorProcessor::updateFilter (float laneValue, float depth, int type, float reso) noexcept
{
    // A sáv értéke (0..1) logaritmikusan képződik le 30 Hz..18 kHz közé.
    // A mélység a "semleges" állapot (LP: nyitott, HP: nyitott) és a leképezett vágási frekvencia között vált.
    static const float logLo = std::log (30.0f);
    static const float logHi = std::log (18000.0f);

    const float mapped  = logLo + laneValue * (logHi - logLo);
    const float neutral = (type == 1) ? std::log (20.0f) : logHi;
    const float logFc   = neutral + depth * (mapped - neutral);

    const float fc = juce::jlimit (20.0f, (float) (0.45 * fs), std::exp (logFc));
    const float q  = 0.707f + reso * reso * 8.0f * depth;   // rezonancia csak a mélységgel együtt hallatszik

    const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) fs);
    svfK  = 1.0f / q;
    svfA1 = 1.0f / (1.0f + g * (g + svfK));
    svfA2 = g * svfA1;
    svfA3 = g * svfA2;
}

float ShapetratorProcessor::processFilter (int ch, float x, int type, float depth) noexcept
{
    const float v3 = x - svfIc2[ch];
    const float v1 = svfA1 * svfIc1[ch] + svfA2 * v3;
    const float v2 = svfIc2[ch] + svfA2 * svfIc1[ch] + svfA3 * v3;
    svfIc1[ch] = 2.0f * v1 - svfIc1[ch];
    svfIc2[ch] = 2.0f * v2 - svfIc2[ch];

    switch (type)
    {
        case 1:  return x - svfK * v1 - v2;               // highpass
        case 2:  return x + depth * (svfK * v1 - x);       // bandpass (mélységgel keverve)
        default: return v2;                                // lowpass
    }
}

//==============================================================================
void ShapetratorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh      = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();

    if (numCh == 0 || numSamples == 0)
        return;

    // --- Paraméterek ---------------------------------------------------------
    const int steps    = getActiveSteps();
    const int rateIdx  = juce::jlimit (0, numRates - 1, (int) std::lround (rateRaw->load()));
    const int ftype    = juce::jlimit (0, 2, (int) std::lround (ftypeRaw->load()));
    const float smooth = smoothRaw->load();
    const float reso   = resoRaw->load();

    for (int l = 0; l < numLanes; ++l)
        depthSm[l].setTargetValue (depthRaw[l]->load());

    mixSm.setTargetValue (mixRaw->load());

    // --- Host idő ----------------------------------------------------------------
    double bpm = 120.0, ppq = 0.0;
    bool playing = false, havePpq = false;

    if (auto* ph = getPlayHead())
    {
        if (auto info = ph->getPosition())
        {
            if (auto b = info->getBpm())
                bpm = *b;

            playing = info->getIsPlaying();

            if (auto p = info->getPpqPosition())
            {
                ppq = *p;
                havePpq = true;
            }
        }
    }

    bpm = juce::jlimit (20.0, 400.0, bpm);

    const double stepsPerBeat   = stepsPerBeatTable[rateIdx];
    const double stepsPerSample = (bpm / (60.0 * fs)) * stepsPerBeat;
    const bool synced           = playing && havePpq;
    double pos                  = synced ? ppq * stepsPerBeat : freeRunPos;

    // Simítás: 0 = éles lépcső (csak kattanásmentesítés), 1 = majdnem teljes lépéshossznyi átmenet
    const double stepSeconds   = 1.0 / (stepsPerSample * fs);
    const double smoothSeconds = 0.0006 + (double) smooth * 0.9 * stepSeconds;
    const float coeff          = (float) std::exp (-1.0 / (smoothSeconds * fs));

    float* chan[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    int filterCounter = 0;
    int idx = 0;

    for (int i = 0; i < numSamples; ++i, pos += stepsPerSample)
    {
        idx = (int) (((long long) std::floor (pos) % steps + steps) % steps);

        float depth[numLanes];

        for (int l = 0; l < numLanes; ++l)
        {
            const float target = stepRaw[l][idx]->load();
            laneSm[l] = target + coeff * (laneSm[l] - target);
            depth[l]  = depthSm[l].getNextValue();
        }

        const float mix = mixSm.getNextValue();

        if (filterCounter-- <= 0)
        {
            filterCounter = 7;
            updateFilter (laneSm[Filter], depth[Filter], ftype, reso);
        }

        float dry[2], wet[2];

        for (int ch = 0; ch < numCh; ++ch)
            dry[ch] = wet[ch] = chan[ch][i];

        // 1) Filter
        for (int ch = 0; ch < numCh; ++ch)
            wet[ch] = processFilter (ch, wet[ch], ftype, depth[Filter]);

        // 2) Crush (bit- és mintavételi frekvencia csökkentés)
        const float cAmt = juce::jlimit (0.0f, 1.0f, depth[Crush] * laneSm[Crush]);

        if (cAmt > 0.0005f)
        {
            const float levels = std::exp2 (15.0f - 12.5f * cAmt);
            const float hold   = 1.0f + cAmt * cAmt * 28.0f;

            crushCounter += 1.0f;

            if (crushCounter >= hold)
            {
                crushCounter = std::fmod (crushCounter, hold);

                for (int ch = 0; ch < numCh; ++ch)
                    crushHeld[ch] = std::round (wet[ch] * levels) / levels;
            }

            const float cm = juce::jmin (1.0f, cAmt * 6.0f);

            for (int ch = 0; ch < numCh; ++ch)
                wet[ch] += cm * (crushHeld[ch] - wet[ch]);
        }
        else
        {
            crushCounter = 0.0f;

            for (int ch = 0; ch < numCh; ++ch)
                crushHeld[ch] = wet[ch];
        }

        // 3) Pan (csak sztereó jelnél), egyenlő teljesítményű, középen egységnyi erősítés
        if (numCh == 2)
        {
            const float p   = juce::jlimit (-1.0f, 1.0f, depth[Pan] * (2.0f * laneSm[Pan] - 1.0f));
            const float ang = (p + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
            wet[0] *= juce::MathConstants<float>::sqrt2 * std::cos (ang);
            wet[1] *= juce::MathConstants<float>::sqrt2 * std::sin (ang);
        }

        // 4) Volume
        const float gain = 1.0f - depth[Volume] * (1.0f - laneSm[Volume]);

        for (int ch = 0; ch < numCh; ++ch)
        {
            wet[ch] *= gain;
            chan[ch][i] = dry[ch] + mix * (wet[ch] - dry[ch]);
        }
    }

    // Pozíció megőrzése (mod 720720 = lcm(1..16), így minden lépésszámnál folytonos marad)
    pos = std::fmod (pos, 720720.0);

    if (pos < 0.0)
        pos += 720720.0;

    freeRunPos = pos;
    currentStep.store (idx);
}

//==============================================================================
namespace
{
    using Pattern = std::array<float, maxSteps>;

    Pattern makePattern (int kind, juce::Random& r)
    {
        Pattern p {};

        const int cycles = 1 << r.nextInt (3);          // 1, 2 vagy 4 szinusz-periódus
        const int hits   = 3 + r.nextInt (7);           // euklideszi ütések száma
        const int offset = r.nextInt (maxSteps);
        const int group  = 1 << r.nextInt (3);          // 1, 2 vagy 4 lépéses csoportok
        const float hi   = 0.7f + 0.3f * r.nextFloat();
        const float lo   = 0.25f * r.nextFloat();

        for (int i = 0; i < maxSteps; ++i)
        {
            const float t = (float) i / (float) (maxSteps - 1);

            switch (kind)
            {
                case 0:  p[(size_t) i] = r.nextFloat(); break;                                          // véletlen
                case 1:  p[(size_t) i] = r.nextFloat() < 0.55f ? 1.0f : 0.0f; break;                    // gate
                case 2:  p[(size_t) i] = t; break;                                                      // felfelé
                case 3:  p[(size_t) i] = 1.0f - t; break;                                               // lefelé
                case 4:  p[(size_t) i] = 0.5f + 0.5f * std::sin (juce::MathConstants<float>::twoPi
                                                                  * (float) cycles * (float) i / (float) maxSteps); break; // szinusz
                case 5:  p[(size_t) i] = (((i + offset) * hits) % maxSteps) < hits ? 1.0f : 0.15f * r.nextFloat(); break; // euklideszi
                case 6:  p[(size_t) i] = (i % 2 == 0) ? hi : lo; break;                                 // váltakozó
                default: p[(size_t) i] = ((i / group) % 2 == 0) ? hi : lo; break;                       // csoportos stutter
            }
        }

        return p;
    }
}

void ShapetratorProcessor::randomise()
{
    auto& r = juce::Random::getSystemRandom();

    auto setPlain = [this] (const juce::String& id, float plainValue)
    {
        if (auto* p = apvts.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
            p->endChangeGesture();
        }
    };

    static const int volKinds[] = { 0, 1, 1, 5, 5, 6, 7 };
    static const int fltKinds[] = { 0, 2, 3, 4, 4, 6, 7 };
    static const int panKinds[] = { 0, 4, 4, 6, 7 };

    Pattern patterns[numLanes];
    patterns[Volume] = makePattern (volKinds[r.nextInt ((int) std::size (volKinds))], r);
    patterns[Filter] = makePattern (fltKinds[r.nextInt ((int) std::size (fltKinds))], r);
    patterns[Pan]    = makePattern (panKinds[r.nextInt ((int) std::size (panKinds))], r);

    // Crush: ritka, "villanásszerű" mintázat
    const bool crushFlatLow = r.nextFloat() < 0.25f;
    for (auto& v : patterns[Crush])
        v = crushFlatLow ? 0.1f + 0.2f * r.nextFloat() : (r.nextFloat() < 0.6f ? 0.0f : r.nextFloat());

    // Mely sávok legyenek aktívak? (legalább kettő mindig)
    const float probability[numLanes] = { 0.85f, 0.80f, 0.50f, 0.40f };
    bool active[numLanes];
    int activeCount = 0;

    for (int l = 0; l < numLanes; ++l)
    {
        active[l] = r.nextFloat() < probability[l];
        activeCount += active[l] ? 1 : 0;
    }

    if (activeCount < 2)
        active[Volume] = active[Filter] = true;

    for (int l = 0; l < numLanes; ++l)
    {
        setPlain (depthIds[l], active[l] ? 0.4f + 0.6f * r.nextFloat() : 0.0f);

        for (int s = 0; s < maxSteps; ++s)
            setPlain (stepId (l, s), patterns[l][(size_t) s]);
    }

    static const int rateChoices[] = { 2, 2, 4, 4, 4, 5, 3 };
    static const int stepChoices[] = { 8, 12, 16, 16, 16 };

    setPlain ("rate",   (float) rateChoices[r.nextInt ((int) std::size (rateChoices))]);
    setPlain ("steps",  (float) stepChoices[r.nextInt ((int) std::size (stepChoices))]);
    setPlain ("smooth", r.nextFloat() < 0.5f ? r.nextFloat() * 0.15f : r.nextFloat() * 0.7f);
    setPlain ("reso",   0.1f + 0.6f * r.nextFloat());

    const float f = r.nextFloat();
    setPlain ("ftype",  f < 0.55f ? 0.0f : (f < 0.75f ? 1.0f : 2.0f));
}

//==============================================================================
juce::AudioProcessorEditor* ShapetratorProcessor::createEditor()
{
    return new ShapetratorEditor (*this);
}

void ShapetratorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void ShapetratorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShapetratorProcessor();
}
