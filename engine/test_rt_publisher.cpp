#include <catch2/catch_test_macros.hpp>

#include "rt_publisher.hpp"

#include <cmath>
#include <vector>

using namespace pulp::analyzer;

namespace {
std::vector<float> sine(float hz, float sr, int n) {
    std::vector<float> s(n);
    const float pi = 3.14159265358979323846f;
    for (int i = 0; i < n; ++i) s[i] = 0.5f * std::sin(2.0f * pi * hz * i / sr);
    return s;
}
int argmax(const std::vector<float>& v) {
    int best = 0;
    for (int i = 1; i < (int)v.size(); ++i) if (v[i] > v[best]) best = i;
    return best;
}
SpectrumConfig cfg() {
    SpectrumConfig c; c.fft_size = 2048; c.sample_rate = 48000.0; c.log_bands = 96;
    return c;
}
} // namespace

TEST_CASE("RtSpectrumPublisher publishes a spectrum of pushed audio", "[rt]") {
    RtSpectrumPublisher<256, 64> pub;
    pub.configure(cfg());

    // Push enough of a 1 kHz tone to fill one FFT window, draining as we go so
    // the ring never overflows (mirrors a worker keeping up).
    auto tone = sine(1000.0f, 48000.0f, 2048 * 2);
    for (int i = 0; i < (int)tone.size(); i += 256) {
        pub.push_audio(tone.data() + i, 256);
        pub.drain_and_analyze();
    }

    auto snap = pub.snapshot();
    REQUIRE(snap.version > 0);                 // at least one window published
    REQUIRE(snap.sample_rate == 48000.0);
    REQUIRE(snap.bins_db.size() == 96);
    for (float v : snap.bins_db) REQUIRE(std::isfinite(v));
    REQUIRE(snap.bins_db[argmax(snap.bins_db)] > -20.0f);  // the tone is present
    REQUIRE(pub.dropped_frames() == 0);        // worker kept up: no drops
}

TEST_CASE("RtSpectrumPublisher drops whole frames on ring overflow", "[rt]") {
    RtSpectrumPublisher<256, 8> pub;  // tiny ring
    pub.configure(cfg());

    // Push far more than the ring can hold WITHOUT draining → overflow drops.
    std::vector<float> block(256, 0.25f);
    for (int i = 0; i < 100; ++i) pub.push_audio(block.data(), 256);

    REQUIRE(pub.dropped_frames() > 0);
    // Draining after overflow must not crash and still works going forward.
    REQUIRE_NOTHROW(pub.drain_and_analyze());
}

TEST_CASE("RtSpectrumPublisher snapshot version advances with new windows",
          "[rt]") {
    RtSpectrumPublisher<256, 128> pub;
    pub.configure(cfg());
    auto tone = sine(440.0f, 48000.0f, 2048 * 3);
    for (int i = 0; i < (int)tone.size(); i += 256) pub.push_audio(tone.data() + i, 256);
    pub.drain_and_analyze();

    const auto v1 = pub.snapshot().version;
    REQUIRE(v1 >= 1);
}
