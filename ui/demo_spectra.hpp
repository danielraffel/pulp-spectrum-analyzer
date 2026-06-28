#pragma once

// Deterministic demo spectra shared by the UI baseline bake + test, so both
// render identical inputs. Pure: a fixed sine through the analysis engine and a
// fixed before/after diff — no randomness, no time dependence.

#include "../engine/spectrum_diff.hpp"
#include "../engine/spectrum_engine.hpp"

#include <cmath>
#include <vector>

namespace pulp::analyzer::demo {

inline std::vector<float> sine(float hz, float sr, int n) {
    std::vector<float> s(n);
    const float pi = 3.14159265358979323846f;
    for (int i = 0; i < n; ++i) s[i] = 0.5f * std::sin(2.0f * pi * hz * i / sr);
    return s;
}

inline SpectrumEngine make_engine() {
    SpectrumEngine e;
    SpectrumConfig cfg;
    cfg.fft_size = 4096;
    cfg.sample_rate = 48000.0;
    cfg.log_bands = 96;
    e.configure(cfg);
    return e;
}

// "Normal" view input: the magnitude spectrum of a 1 kHz tone.
inline std::vector<float> normal_bins() {
    auto e = make_engine();
    auto tone = sine(1000.0f, 48000.0f, 4096);
    return e.analyze(tone.data(), static_cast<int>(tone.size()));  // copy of internal
}

// "Diff" view input: per-band delta of a 1 kHz "after" vs a 250 Hz "before".
inline std::vector<float> diff_bins() {
    auto e = make_engine();
    auto before = e.analyze(sine(250.0f, 48000.0f, 4096).data(), 4096);
    auto after = e.analyze(sine(1000.0f, 48000.0f, 4096).data(), 4096);
    auto d = diff_spectra(before, after, 1.0f);
    return d.delta_db;
}

} // namespace pulp::analyzer::demo
