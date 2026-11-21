// Test BAKR CALIBRE : simule une pièce (modes, délai, écart G/D) entre l'instance Master et l'instance Micro
#include <JuceHeader.h>
#include "../Common/BakrDSP.h"
#include <cstdio>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();
juce::String calibreStart (juce::AudioProcessor*);
int calibreState (juce::AudioProcessor*);
juce::String calibreReport (juce::AudioProcessor*);

struct Room
{
    bakr::Biquad m1, m2, d1, hf;
    std::vector<float> dl; int w = 0, delay; float g;
    Room (double fs, int del, float gain, float mode2) : dl (8192, 0.0f), delay (del), g (gain)
    {
        m1.peak (fs, 55.0, 6.0, 9.0); m2.peak (fs, 110.0, 4.0, mode2); d1.peak (fs, 190.0, 3.0, -6.0); hf.highShelf (fs, 6000.0, -2.0, 0.7);
    }
    float process (float x)
    {
        dl[(size_t) w] = x;
        const float y = dl[(size_t) ((w - delay) & 8191)] + 0.3f * dl[(size_t) ((w - delay - 300) & 8191)];
        w = (w + 1) & 8191;
        return g * hf.process (d1.process (m2.process (m1.process (y))));
    }
};

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000; const int bs = 512;
    std::unique_ptr<juce::AudioProcessor> master (createPluginFilter()), mic (createPluginFilter());
    for (auto* p : { master.get(), mic.get() }) { p->setPlayConfigDetails (2, 2, fs, bs); p->prepareToPlay (fs, bs); }
    auto setP = [] (juce::AudioProcessor* p, const char* id, float v)
    {
        for (auto* prm : p->getParameters())
            if (auto* wp = dynamic_cast<juce::RangedAudioParameter*> (prm); wp && wp->getParameterID() == id)
                wp->setValueNotifyingHost (wp->convertTo0to1 (v));
    };
    setP (mic.get(), "role", 1);
    setP (master.get(), "micType", 0);

    // latence 300 éch. + distance : G 2.0 ms, D 2.6 ms (≈ 20 cm de plus), D 2 dB plus faible
    Room roomL (fs, 300 + 96, 0.5f, 5.0f), roomR (fs, 300 + 125, 0.5f * 0.794f, 3.0f);
    juce::Random rnd (5);
    juce::AudioBuffer<float> bm (2, bs), bmic (2, bs);
    juce::MidiBuffer mb;
    auto cycle = [&]
    {
        for (int i = 0; i < bs; ++i) { const float v = 0.1f * (rnd.nextFloat() * 2 - 1); bm.setSample (0, i, v); bm.setSample (1, i, v); }
        mic->processBlock (bmic, mb);           // la piste micro passe d'abord
        master->processBlock (bm, mb);
        for (int i = 0; i < bs; ++i)
        {
            const float m = roomL.process (bm.getSample (0, i)) + roomR.process (bm.getSample (1, i)) + 1e-4f * (rnd.nextFloat() * 2 - 1);
            bmic.setSample (0, i, m); bmic.setSample (1, i, 0.0f);   // micro sur une seule voie
        }
        for (int c = 0; c < 2; ++c) for (int i = 0; i < bs; ++i) if (! std::isfinite (bm.getSample (c, i))) { std::printf ("NaN!\n"); std::exit (1); }
    };
    for (int k = 0; k < 20; ++k) cycle();

    for (int pos = 0; pos < 3; ++pos)
    {
        auto err = calibreStart (master.get());
        if (err.isNotEmpty()) { std::printf ("start error: %s\n", err.toRawUTF8()); return 1; }
        int guard = 0;
        do { cycle(); if (calibreState (master.get()) == 3) juce::Thread::sleep (5); } while (calibreState (master.get()) != 0 && ++guard < 20000);
        std::printf ("pos %d done (%d blocks)\n", pos + 1, guard);
    }
    juce::Thread::sleep (200);
    std::printf ("%s\n", calibreReport (master.get()).toRawUTF8());

    // la correction tourne sur de la musique sans NaN
    float pk = 0;
    for (int k = 0; k < 200; ++k) { cycle(); for (int i = 0; i < bs; ++i) pk = juce::jmax (pk, std::abs (bm.getSample (0, i))); }
    std::printf ("out peak with correction: %.1f dB (in -20 dB)\n", juce::Decibels::gainToDecibels (pk));

    // état sauvegardé / rechargé
    juce::MemoryBlock st; master->getStateInformation (st);
    std::unique_ptr<juce::AudioProcessor> again (createPluginFilter());
    again->setPlayConfigDetails (2, 2, fs, bs); again->prepareToPlay (fs, bs);
    again->setStateInformation (st.getData(), (int) st.getSize());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    std::printf ("reloaded state %d bytes:\n%s\n", (int) st.getSize(), calibreReport (again.get()).substring (0, 400).toRawUTF8());

    for (int pass = 0; pass < 2; ++pass)
    {
        auto* p = pass == 0 ? master.get() : mic.get();
        std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
        for (int k = 0; k < 10; ++k) cycle();
        juce::MessageManager::getInstance()->runDispatchLoopUntil (400);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File f (juce::String ("/home/claude/shots/Calibre") + (pass == 0 ? "" : "_Micro") + ".png");
        f.getParentDirectory().createDirectory(); f.deleteFile();
        juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    }
    return 0;
}
