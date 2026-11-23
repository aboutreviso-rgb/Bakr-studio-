#include <JuceHeader.h>
#include <cstdio>
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000; const int bs = 512;
    std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
    p->setPlayConfigDetails (2, 2, fs, bs); p->prepareToPlay (fs, bs);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (100);
    juce::AudioBuffer<float> b (2, bs); juce::MidiBuffer mb; juce::Random rnd (1);
    double eL = 0, eR = 0, eIn = 0; float pk = 0;
    for (int k = 0; k < 400; ++k)
    {
        for (int i = 0; i < bs; ++i) { const float v = 0.25f * (rnd.nextFloat() * 2 - 1); b.setSample (0, i, v); b.setSample (1, i, 0.0f); eIn += v * v; }
        p->processBlock (b, mb);
        for (int i = 0; i < bs; ++i) { const float l = b.getSample (0, i), r = b.getSample (1, i); if (! std::isfinite (l) || ! std::isfinite (r)) { std::puts ("NaN"); return 1; } eL += l * l; eR += r * r; pk = std::max (pk, std::abs (l)); }
    }
    std::printf ("in %.1f  outL %.1f  outR(crossfeed) %.1f dB  peak %.1f\n", 10*std::log10 (eIn), 10*std::log10 (eL), 10*std::log10 (eR), juce::Decibels::gainToDecibels (pk));
    auto setP = [&] (const char* id, float v) { for (auto* prm : p->getParameters()) if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm); rp && rp->getParameterID() == id) rp->setValueNotifyingHost (rp->convertTo0to1 (v)); };
    setP ("emuOn", 0); setP ("bass", 0); setP ("treble", 0);
    for (int tg = 0; tg < 3; ++tg) for (int ca = 0; ca < 2; ++ca)
    {
        setP ("target", (float) tg); setP ("calOn", (float) ca);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
        double e1 = 0, e2 = 0;
        for (int k = 0; k < 40; ++k)
        {
            for (int i = 0; i < bs; ++i) { const float v = 0.1f * std::sin (2 * M_PI * 1000.0 * (k * bs + i) / fs); b.setSample (0, i, v); b.setSample (1, i, v); e1 += v * v; }
            p->processBlock (b, mb);
            double eb = 0; for (int i = 0; i < bs; ++i) { e2 += b.getSample (0, i) * b.getSample (0, i); eb += b.getSample (0, i) * b.getSample (0, i); }
            if (tg == 1 && ca == 0 && k % 4 == 0) std::printf ("   blk %d %.1f dB\n", k, 10 * std::log10 (eb / (bs * 0.005)));
        }
        std::printf ("target %d cal %d : gain 1k %.1f dB\n", tg, ca, 10 * std::log10 (e2 / e1));
    }
    setP ("emuOn", 1); setP ("calOn", 1);
    for (int sp = 0; sp < 5; ++sp) for (int rm = 0; rm < 6; ++rm) for (int mo = 0; mo < 6; ++mo)
    {
        setP ("speaker", (float) sp); setP ("roomType", (float) rm); setP ("monitor", (float) mo); setP ("room", 80); setP ("target", 0.0f); setP ("bass", 4); setP ("treble", -3);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (5);
        float m = 0;
        for (int k = 0; k < 20; ++k)
        {
            for (int i = 0; i < bs; ++i) { const float v = 0.3f * (rnd.nextFloat() * 2 - 1); b.setSample (0, i, v); b.setSample (1, i, v * 0.5f); }
            p->processBlock (b, mb);
            for (int c = 0; c < 2; ++c) for (int i = 0; i < bs; ++i) { const float v = b.getSample (c, i); if (! std::isfinite (v)) { std::printf ("NaN sp%d rm%d mo%d\n", sp, rm, mo); return 1; } m = std::max (m, std::abs (v)); }
        }
        if (mo == 0) std::printf ("sp%d rm%d peak %.1f dB\n", sp, rm, juce::Decibels::gainToDecibels (m));
    }
    setP ("speaker", 1); setP ("roomType", 0); setP ("monitor", 0); setP ("room", 25); setP ("target", 0); setP ("bass", 0); setP ("treble", 0);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f ("/home/claude/shots/Casque.png"); f.deleteFile();
    juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
