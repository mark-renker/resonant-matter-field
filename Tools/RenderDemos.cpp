#include "DSP/RMFEngine.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

namespace
{
struct NoteEvent { double time; int note; bool on; float velocity; };

bool render (const juce::File& output, const juce::File& tuningFile, rmf::Parameters p,
             const std::vector<NoteEvent>& events, double seconds)
{
    constexpr double sr = 48000.0;
    rmf::ScalaTuning tuning;
    if (tuningFile.existsAsFile())
    {
        const auto result = tuning.loadFromFile (tuningFile);
        if (! result.ok) { std::cerr << result.message << "\n"; return false; }
    }
    rmf::Engine engine; engine.prepare (sr);
    const int samples = (int) (seconds * sr);
    juce::AudioBuffer<float> audio (2, samples); audio.clear();
    size_t eventIndex = 0;
    float peak = 0.0f;
    for (int i = 0; i < samples; ++i)
    {
        const double t = i / sr;
        while (eventIndex < events.size() && events[eventIndex].time <= t)
        {
            const auto& e = events[eventIndex++];
            if (e.on) engine.noteOn (e.note, e.velocity, p, tuning); else engine.noteOff (e.note);
        }
        const auto s = engine.renderSample (p, tuning);
        audio.setSample (0, i, s[0]); audio.setSample (1, i, s[1]);
        peak = juce::jmax (peak, std::abs (s[0]), std::abs (s[1]));
    }
    if (peak > 0.0f)
        audio.applyGain (juce::jmin (1.0f, 0.88f / peak));

    output.deleteFile();
    auto stream = output.createOutputStream();
    if (stream == nullptr) return false;
    juce::WavAudioFormat format;
    std::unique_ptr<juce::AudioFormatWriter> writer (format.createWriterFor (stream.release(), sr, 2, 24, {}, 0));
    if (writer == nullptr) return false;
    return writer->writeFromAudioSampleBuffer (audio, 0, samples);
}
}

int main (int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: rmf_render output-directory\n"; return 1; }
    const juce::File out (argv[1]); out.createDirectory();
    const auto tuningDir = juce::File::getCurrentWorkingDirectory().getChildFile ("Tunings");

    rmf::Parameters p;
    p.polyphony=8; p.modeCount=32; p.outputGain=1.8f;
    std::vector<NoteEvent> sparse {{0.1,45,true,.88f},{2.2,52,true,.72f},{4.3,58,true,.76f},{6.6,63,true,.70f},{8.8,45,false,0},{9.0,52,false,0},{9.2,58,false,0},{9.4,63,false,0}};
    p.density=3.2f; p.durationMs=4.0f; p.force=.84f; p.roughness=.12f; p.damping=5.8f; p.tuningMode=2; p.tuningSpread=2; p.radiationWidth=.92f; p.dispersion=.01f;
    if (! render (out.getChildFile("01_19edo_sparse_alloy.wav"), tuningDir.getChildFile("19edo.scl"), p, sparse, 12.0)) return 2;

    std::vector<NoteEvent> cloud {{0.1,38,true,.72f},{0.8,45,true,.62f},{1.5,50,true,.56f},{9.2,38,false,0},{9.4,45,false,0},{9.6,50,false,0}};
    p.density=185.0f; p.durationMs=31.0f; p.force=.26f; p.roughness=.88f; p.positionSpread=.44f; p.trajectoryDepth=.38f; p.trajectoryRate=.47f; p.damping=.82f; p.tuningMode=1; p.tuningSpread=1; p.radiationWidth=.76f; p.activeImpedance=.08f;
    if (! render (out.getChildFile("02_just_friction_cloud.wav"), tuningDir.getChildFile("just_major.scl"), p, cloud, 12.0)) return 3;

    std::vector<NoteEvent> tritave {{0.1,41,true,.82f},{1.7,54,true,.68f},{3.5,67,true,.58f},{7.8,41,false,0},{8.2,54,false,0},{8.6,67,false,0}};
    p.density=8.0f; p.durationMs=12.0f; p.force=.62f; p.roughness=.33f; p.positionSpread=.24f; p.trajectoryDepth=.26f; p.trajectoryRate=.19f; p.damping=3.4f; p.tuningMode=2; p.tuningSpread=1; p.dispersion=0.0f; p.radiationWidth=.86f; p.activeImpedance=.18f;
    if (! render (out.getChildFile("03_bohlen_pierce_constellation.wav"), tuningDir.getChildFile("bohlen_pierce_13edt.scl"), p, tritave, 12.0)) return 4;

    std::vector<NoteEvent> active {{0.1,36,true,.68f},{2.4,48,true,.62f},{4.8,55,true,.54f},{8.7,36,false,0},{9.0,48,false,0},{9.3,55,false,0}};
    p.density=12.0f; p.durationMs=15.0f; p.force=.38f; p.roughness=.27f; p.positionSpread=.16f; p.trajectoryDepth=.14f; p.damping=6.8f; p.coupling=.79f; p.loading=.18f; p.inductance=.65f; p.activeImpedance=.68f; p.tuningMode=1; p.radiationWidth=.94f;
    if (! render (out.getChildFile("04_active_coil_bloom.wav"), tuningDir.getChildFile("19edo.scl"), p, active, 12.0)) return 5;
    std::cout << "Rendered four RMF demonstrations to " << out.getFullPathName() << "\n";
    return 0;
}
