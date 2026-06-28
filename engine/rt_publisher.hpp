#pragma once

// RtSpectrumPublisher — lock-free audio→worker→UI handoff (layer 3).
//
// The realtime contract for the analyzer: the audio thread only copies samples
// into a bounded lock-free ring (no allocation, no locks, no FFT). A worker
// drains the ring, runs the (expensive) FFT analysis OFF the audio callback, and
// publishes the latest spectrum through a TripleBuffer the UI reads. Ring
// overflow drops whole frames (counted), never partial samples or torn data.
//
// This models the handoff with Pulp's own lock-free primitives
// (pulp::runtime::SpscQueue + TripleBuffer). It is in-process; cross-process
// shared memory (so a later instance can read an earlier one's spectrum) is a
// separate, larger layer — see ROADMAP.md.

#include "spectrum_engine.hpp"
#include "spectrum_snapshot.hpp"

#include <pulp/runtime/spsc_queue.hpp>
#include <pulp/runtime/triple_buffer.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace pulp::analyzer {

// FrameSize = samples per ring slot; RingCapacity = number of slots.
template <int FrameSize = 256, std::size_t RingCapacity = 64>
class RtSpectrumPublisher {
    static_assert(FrameSize > 0, "FrameSize must be > 0");

public:
    using Frame = std::array<float, FrameSize>;

    void configure(const SpectrumConfig& cfg) {
        engine_.configure(cfg);
        sample_rate_ = cfg.sample_rate;
        window_.assign(static_cast<std::size_t>(cfg.fft_size), 0.0f);
        fft_size_ = cfg.fft_size;
        filled_ = 0;
    }

    // ── Audio thread ─────────────────────────────────────────────────────
    // Copy a mono block into the ring as FrameSize-sized frames. No allocation
    // or locking. A frame that doesn't fit is dropped whole (counted).
    void push_audio(const float* mono, int n) {
        int i = 0;
        while (i < n) {
            const int take = std::min(FrameSize, n - i);
            Frame f{};
            for (int k = 0; k < take; ++k) f[static_cast<std::size_t>(k)] = mono[i + k];
            // Partial tail frames are zero-padded — fine for a streaming tap.
            if (!ring_.try_push(f)) {
                dropped_frames_.fetch_add(1, std::memory_order_relaxed);
            }
            i += take;
        }
    }

    // ── Worker ───────────────────────────────────────────────────────────
    // Drain available frames into the analysis window; whenever a full window
    // is accumulated, run the FFT and publish a fresh snapshot. Returns the
    // number of snapshots published this call.
    int drain_and_analyze() {
        if (fft_size_ <= 0 || window_.empty()) return 0;  // not configured yet
        int published = 0;
        while (auto frame = ring_.try_pop()) {
            for (float s : *frame) {
                window_[filled_++] = s;
                if (filled_ == static_cast<std::size_t>(fft_size_)) {
                    const auto& bins = engine_.analyze(window_.data(), fft_size_);
                    SpectrumSnapshot snap;
                    snap.bins_db = bins;  // copy
                    snap.version = ++version_;
                    snap.sample_rate = sample_rate_;
                    snapshot_.write(snap);
                    filled_ = 0;
                    ++published;
                }
            }
        }
        return published;
    }

    // ── UI thread ────────────────────────────────────────────────────────
    SpectrumSnapshot snapshot() { return snapshot_.read(); }

    std::uint64_t dropped_frames() const {
        return dropped_frames_.load(std::memory_order_relaxed);
    }

private:
    pulp::runtime::SpscQueue<Frame, RingCapacity> ring_;
    pulp::runtime::TripleBuffer<SpectrumSnapshot> snapshot_;
    SpectrumEngine engine_;
    std::vector<float> window_;
    int fft_size_ = 0;
    std::size_t filled_ = 0;
    double sample_rate_ = 0.0;
    std::uint64_t version_ = 0;
    std::atomic<std::uint64_t> dropped_frames_{0};
};

} // namespace pulp::analyzer
