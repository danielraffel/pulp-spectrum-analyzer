#pragma once

// SpectrumSnapshot — the latest published spectrum, shared by the RT publisher
// (writer) and the UI / cross-instance registry (readers). Kept in its own tiny
// header so the registry doesn't have to pull in the FFT engine.

#include <cstdint>
#include <vector>

namespace pulp::analyzer {

struct SpectrumSnapshot {
    std::vector<float> bins_db;
    std::uint64_t version = 0;   // increments each publish; UI detects freshness
    double sample_rate = 0.0;
};

} // namespace pulp::analyzer
