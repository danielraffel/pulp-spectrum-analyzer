#include <catch2/catch_test_macros.hpp>

#include "spectrum_engine.hpp"

#include <cmath>
#include <vector>

using pulp::analyzer::SpectrumConfig;
using pulp::analyzer::SpectrumEngine;

namespace {

constexpr float kPi = 3.14159265358979323846f;

std::vector<float> sine(float hz, float sr, int n, float amp = 0.5f) {
    std::vector<float> s(n);
    for (int i = 0; i < n; ++i) {
        s[i] = amp * std::sin(2.0f * kPi * hz * i / sr);
    }
    return s;
}

int argmax(const std::vector<float>& v) {
    int best = 0;
    for (int i = 1; i < static_cast<int>(v.size()); ++i) {
        if (v[i] > v[best]) best = i;
    }
    return best;
}

SpectrumEngine make_engine() {
    SpectrumEngine e;
    SpectrumConfig cfg;
    cfg.fft_size = 4096;
    cfg.sample_rate = 48000.0;
    cfg.log_bands = 96;
    e.configure(cfg);
    return e;
}

} // namespace

TEST_CASE("SpectrumEngine peaks at the tone frequency", "[spectrum]") {
    auto e = make_engine();
    auto tone = sine(1000.0f, 48000.0f, 4096);
    const auto& spec = e.analyze(tone.data(), static_cast<int>(tone.size()));

    REQUIRE(static_cast<int>(spec.size()) == e.band_count());
    for (float v : spec) REQUIRE(std::isfinite(v));

    const int peak = argmax(spec);
    const float center = e.band_center_hz(peak);
    // Log bands over 20..20000 are ~7.5% wide; the peak band must sit on 1 kHz.
    REQUIRE(center > 900.0f);
    REQUIRE(center < 1100.0f);
    REQUIRE(spec[peak] > -20.0f); // well above the noise floor
}

TEST_CASE("SpectrumEngine reports the floor for silence", "[spectrum]") {
    auto e = make_engine();
    std::vector<float> silence(4096, 0.0f);
    const auto& spec = e.analyze(silence.data(), static_cast<int>(silence.size()));
    for (float v : spec) {
        REQUIRE(std::isfinite(v));
        REQUIRE(v <= -119.0f); // at/near the configured -120 dB floor
    }
}

TEST_CASE("SpectrumEngine distinguishes two tones", "[spectrum]") {
    auto e = make_engine();

    auto low = sine(500.0f, 48000.0f, 4096);
    const int low_peak = argmax(e.analyze(low.data(), 4096));
    const float low_center = e.band_center_hz(low_peak);

    auto high = sine(6000.0f, 48000.0f, 4096);
    const int high_peak = argmax(e.analyze(high.data(), 4096));
    const float high_center = e.band_center_hz(high_peak);

    REQUIRE(low_peak != high_peak);
    REQUIRE(low_center > 440.0f);
    REQUIRE(low_center < 570.0f);
    REQUIRE(high_center > 5400.0f);
    REQUIRE(high_center < 6600.0f);
}
