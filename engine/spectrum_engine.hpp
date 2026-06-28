#pragma once

// SpectrumEngine — log-frequency magnitude analysis (layer 1).
//
// This is the pure, headless-testable analysis layer of the Spectrum Analyzer.
// It takes a window of mono samples and produces a log-frequency magnitude
// spectrum in dB. It does NOT touch the audio thread, MIDI, IPC, or the GUI:
// those are separate layers (see the repo README's layering section). Keeping
// the analysis pure makes it deterministic and unit-testable without a host.
//
// Pipeline: Hann window -> real FFT (pulp::signal::Fft) -> per-FFT-bin linear
// magnitude -> fold into log-spaced output bands (peak magnitude per band) ->
// dB. Log spacing gives the musically meaningful, roughly constant-Q view a
// spectrum display wants; per-band peak keeps narrow tones visible rather than
// averaged away. A sharper constant-Q (sparse-kernel) analysis is a documented
// future refinement; this layer's contract is "log-frequency magnitude in dB".

#include <pulp/signal/fft.hpp>

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

namespace pulp::analyzer {

struct SpectrumConfig {
    int fft_size = 2048;          // power of 2
    double sample_rate = 48000.0;
    int log_bands = 96;           // output resolution
    float min_hz = 20.0f;
    float max_hz = 20000.0f;
    float floor_db = -120.0f;     // value reported for empty/no-energy bands
};

class SpectrumEngine {
public:
    void configure(const SpectrumConfig& cfg) {
        cfg_ = cfg;
        fft_ = pulp::signal::Fft(cfg_.fft_size);
        window_.resize(cfg_.fft_size);
        // Hann window.
        const int n = cfg_.fft_size;
        for (int i = 0; i < n; ++i) {
            window_[i] = 0.5f * (1.0f - std::cos(2.0f * kPi * i / (n - 1)));
        }
        windowed_.assign(cfg_.fft_size, 0.0f);
        freq_.assign(cfg_.fft_size, {});
        linear_.assign(cfg_.fft_size / 2 + 1, 0.0f);

        // Log-spaced band edges across [min_hz, max_hz].
        const float hi = std::min(cfg_.max_hz,
                                  static_cast<float>(cfg_.sample_rate) * 0.5f);
        const float lo = std::clamp(cfg_.min_hz, 1.0f, hi - 1.0f);
        band_edges_.resize(cfg_.log_bands + 1);
        const float log_lo = std::log(lo);
        const float log_hi = std::log(hi);
        for (int b = 0; b <= cfg_.log_bands; ++b) {
            const float t = static_cast<float>(b) / cfg_.log_bands;
            band_edges_[b] = std::exp(log_lo + t * (log_hi - log_lo));
        }
        spectrum_db_.assign(cfg_.log_bands, cfg_.floor_db);
    }

    int band_count() const { return cfg_.log_bands; }

    // Center frequency (Hz) of output band b — geometric mean of its edges.
    float band_center_hz(int b) const {
        return std::sqrt(band_edges_[b] * band_edges_[b + 1]);
    }

    // Analyze `count` mono samples (only the first fft_size are used; fewer are
    // zero-padded). Returns the log-band magnitude spectrum in dB.
    const std::vector<float>& analyze(const float* samples, int count) {
        const int n = cfg_.fft_size;
        for (int i = 0; i < n; ++i) {
            const float s = (i < count) ? samples[i] : 0.0f;
            windowed_[i] = s * window_[i];
        }

        fft_.forward_real(windowed_.data(), freq_.data());

        const int half = n / 2;
        // Normalize by window sum so magnitude is amplitude-meaningful.
        float window_sum = 0.0f;
        for (int i = 0; i < n; ++i) window_sum += window_[i];
        const float norm = (window_sum > 0.0f) ? (2.0f / window_sum) : 1.0f;
        for (int k = 0; k <= half; ++k) {
            linear_[k] = std::abs(freq_[k]) * norm;
        }

        std::fill(spectrum_db_.begin(), spectrum_db_.end(), cfg_.floor_db);
        const float bin_hz = static_cast<float>(cfg_.sample_rate) / n;
        // Fold FFT bins into log bands, keeping the peak magnitude per band.
        std::vector<float> band_peak(cfg_.log_bands, 0.0f);
        for (int k = 1; k <= half; ++k) {
            const float f = k * bin_hz;
            if (f < band_edges_.front() || f >= band_edges_.back()) continue;
            // Binary search for the band whose [edge_b, edge_b+1) contains f.
            int b = static_cast<int>(
                std::upper_bound(band_edges_.begin(), band_edges_.end(), f) -
                band_edges_.begin()) - 1;
            if (b < 0 || b >= cfg_.log_bands) continue;
            band_peak[b] = std::max(band_peak[b], linear_[k]);
        }
        for (int b = 0; b < cfg_.log_bands; ++b) {
            if (band_peak[b] > 0.0f) {
                spectrum_db_[b] = 20.0f * std::log10(band_peak[b]);
            }
        }
        return spectrum_db_;
    }

    const std::vector<float>& spectrum_db() const { return spectrum_db_; }

private:
    static constexpr float kPi = 3.14159265358979323846f;

    SpectrumConfig cfg_;
    pulp::signal::Fft fft_{2048};
    std::vector<float> window_;
    std::vector<float> windowed_;
    std::vector<std::complex<float>> freq_;
    std::vector<float> linear_;
    std::vector<float> band_edges_;
    std::vector<float> spectrum_db_;
};

} // namespace pulp::analyzer
