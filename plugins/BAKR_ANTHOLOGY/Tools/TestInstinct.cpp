// Test BAKR INSTINCT : voix synthétique (harmoniques + formants + « S » + syllabes)
#include <JuceHeader.h>
#include "../Common/BakrDSP.h"
#include <cstdio>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int instinctState (juce::AudioProcessor*);
void instinctListen (juce::AudioProcessor*);
void instinctApply (juce::AudioProcessor*);
juce::String instinctReport (juce::AudioProcessor*);

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000; const int bs = 512;
    std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
    p->setPlayConfigDetails (2, 2, fs, bs); p->prepareToPlay (fs, bs);
    bakr::Biquad f1, f2, f3, sib, mud;
    f1.peak (fs, 700, 4, 12); f2.peak (fs, 1200, 5, 9); f3.peak (fs, 2800, 4, 10); sib.highPass (fs, 6000, 0.7); mud.peak (fs, 300, 2, 6);
    juce::Random rnd (3);
    double ph = 0; long t = 0;
    juce::AudioBuffer<float> b (2, bs); juce::MidiBuffer mb;
    auto gen = [&]
    {
        for (int i = 0; i < bs; ++i, ++t)
        {
            const double sec = t / fs;
            const double f0 = 230.0 * std::pow (2.0, (std::floor (std::fmod (sec * 2.0, 4.0)) * 2.0 - 3.0) / 12.0) * (1 + 0.004 * std::sin (2 * M_PI * 5.5 * sec));
            ph += 2 * M_PI * f0 / fs;
            double v = 0; for (int h = 1; h < 30 && h * f0 < 20000; ++h) v += std::sin (h * ph) / h;
            const double syl = std::fmod (sec * 4.0, 1.0);
            double env = syl < 0.7 ? std::sin (M_PI * syl / 0.7) : 0.0;
            const bool s = std::fmod (sec, 2.0) > 1.85;
            float x = (float) (mud.process (f3.process (f2.process (f1.process ((float) (v * 0.05))))) * env);
            if (s) x = sib.process (rnd.nextFloat() * 2 - 1) * 0.25f;
            b.setSample (0, i, x * 0.5f); b.setSample (1, i, x * 0.5f);
        }
    };
    for (int k = 0; k < 20; ++k) { gen(); p->processBlock (b, mb); }
    instinctListen (p.get());
    int guard = 0;
    while (instinctState (p.get()) == 1 && guard++ < 5000) { gen(); p->processBlock (b, mb); }
    while (instinctState (p.get()) == 2) juce::Thread::sleep (20);
    std::printf ("%s\n", instinctReport (p.get()).toRawUTF8());
    instinctApply (p.get());
    float pk = 0; double e = 0; int cnt = 0;
    for (int k = 0; k < 300; ++k) { gen(); p->processBlock (b, mb); for (int i = 0; i < bs; ++i) { const float v = b.getSample (0, i); if (! std::isfinite (v)) { std::puts ("NaN"); return 1; } pk = std::max (pk, std::abs (v)); e += v * v; ++cnt; } }
    std::printf ("out peak %.1f dB rms %.1f dB\n", juce::Decibels::gainToDecibels (pk), 10 * std::log10 (e / cnt));
    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    for (int k = 0; k < 30; ++k) { gen(); p->processBlock (b, mb); juce::MessageManager::getInstance()->runDispatchLoopUntil (25); }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f ("/home/claude/shots/Instinct.png"); f.deleteFile();
    juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
