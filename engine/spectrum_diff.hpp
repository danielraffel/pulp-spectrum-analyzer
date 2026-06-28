#pragma once

// SpectrumDiff — before/after spectral comparison (layer 2).
//
// The analyzer's signature feature is comparing two spectra: a "before" curve
// (an earlier instance in the chain, or a stored reference) against the current
// "after" curve, showing per-band boost (after louder) vs cut (after quieter).
// This layer is pure and headless: it takes two dB spectra of equal length and
// produces a per-band delta plus a classification, with a view mode for how a
// UI would present it. No FFT, no GPU, no IPC here — those are other layers.

#include <cstddef>
#include <vector>

namespace pulp::analyzer {

enum class ViewMode {
    normal,  // show only the current ("after") spectrum
    diff,    // show only the boost/cut delta vs the reference
    both,    // show the reference overlaid under the current
};

enum class BandChange { flat, boost, cut };

struct SpectrumDiffResult {
    std::vector<float> delta_db;          // after - before, per band
    std::vector<BandChange> classification;
    bool valid = false;                   // false if inputs mismatched/empty
};

// Compare `after` against `before` (both in dB, same length). A band is a boost
// when after exceeds before by more than `threshold_db`, a cut when it falls
// below by more than `threshold_db`, otherwise flat. Returns valid=false (and
// empty vectors) when the inputs are empty or length-mismatched, so a caller
// can't silently diff incompatible spectra.
inline SpectrumDiffResult diff_spectra(const std::vector<float>& before,
                                       const std::vector<float>& after,
                                       float threshold_db = 1.0f) {
    SpectrumDiffResult r;
    if (before.empty() || before.size() != after.size()) {
        return r; // valid stays false
    }
    const std::size_t n = before.size();
    r.delta_db.resize(n);
    r.classification.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        const float d = after[i] - before[i];
        r.delta_db[i] = d;
        if (d > threshold_db) {
            r.classification[i] = BandChange::boost;
        } else if (d < -threshold_db) {
            r.classification[i] = BandChange::cut;
        } else {
            r.classification[i] = BandChange::flat;
        }
    }
    r.valid = true;
    return r;
}

} // namespace pulp::analyzer
