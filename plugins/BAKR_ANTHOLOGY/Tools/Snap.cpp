// Outil de test : passe un signal dans le plugin (tous les presets) puis capture l'interface
#include <JuceHeader.h>
#include <cstdio>

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter();

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    std::unique_ptr<juce::AudioProcessor> p (createPluginFilter());
    p->setPlayConfigDetails (2, 2, 48000, 512);
    p->prepareToPlay (48000, 512);

    juce::Random rnd (1);
    juce::MidiBuffer mb;
    juce::AudioBuffer<float> buf (2, 512);
    bool bad = false;
    for (int pr = 0; pr < p->getNumPrograms(); ++pr)
    {
        p->setCurrentProgram (pr);
        double ph = 0; float pk = 0, inPk = 0;
        for (int blk = 0; blk < 120; ++blk)
        {
            for (int i = 0; i < 512; ++i)
            {
                // sinus 120 Hz + bruit, enveloppe rythmée, ~ -10 dBFS crête
                const float envl = ((blk / 6) % 2) ? 0.35f : 0.12f;
                const float s = envl * (float) std::sin (ph) + 0.03f * (rnd.nextFloat() * 2 - 1);
                ph += juce::MathConstants<double>::twoPi * 120.0 / 48000.0;
                buf.setSample (0, i, s); buf.setSample (1, i, s * 0.9f);
                inPk = juce::jmax (inPk, std::abs (s));
            }
            p->processBlock (buf, mb);
            for (int c = 0; c < 2; ++c)
                for (int i = 0; i < 512; ++i)
                {
                    const float v = buf.getSample (c, i);
                    if (! std::isfinite (v)) bad = true;
                    pk = juce::jmax (pk, std::abs (v));
                }
        }
        std::printf ("  %-30s in %6.1f dB -> out %6.1f dB\n", p->getProgramName (pr).toRawUTF8(),
                     juce::Decibels::gainToDecibels (inPk), juce::Decibels::gainToDecibels (pk));
    }
    std::printf ("%s: %s\n", SNAP_NAME, bad ? "ERREUR (NaN/Inf)" : "audio OK");

    p->setCurrentProgram (1);
    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    for (int k = 0; k < 40; ++k) { p->processBlock (buf, mb); }
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
    juce::File f (juce::String ("/home/claude/shots/") + SNAP_NAME + ".png");
    f.getParentDirectory().createDirectory();
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat().writeImageToStream (img, os);
    return bad ? 1 : 0;
}
