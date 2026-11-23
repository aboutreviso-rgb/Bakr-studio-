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
    std::unique_ptr<juce::AudioProcessorEditor> ed (p->createEditor());
    juce::MessageManager::getInstance()->runDispatchLoopUntil (300);
    auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    juce::File f ("/home/claude/shots/Casque.png"); f.deleteFile();
    juce::FileOutputStream os (f); juce::PNGImageFormat().writeImageToStream (img, os);
    return 0;
}
