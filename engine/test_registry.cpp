#include <catch2/catch_test_macros.hpp>

#include "registry.hpp"

#include <algorithm>
#include <memory>

using namespace pulp::analyzer;

namespace {
SpectrumSnapshot snap(std::uint64_t version, float peak_db) {
    SpectrumSnapshot s;
    s.version = version;
    s.sample_rate = 48000.0;
    s.bins_db = {peak_db, -80.0f, -80.0f};
    return s;
}
} // namespace

TEST_CASE("Two instances discover and compare spectra by name", "[registry]") {
    // Use a local registry instance to keep the test isolated from any
    // process-global state.
    InstanceRegistry reg;

    auto pre = std::make_unique<RegisteredSpectrumSource>(
        "Pre-EQ", [] { return snap(7, -6.0f); }, reg);
    auto post = std::make_unique<RegisteredSpectrumSource>(
        "Post-EQ", [] { return snap(7, -3.0f); }, reg);

    REQUIRE(reg.size() == 2);
    auto names = reg.names();
    REQUIRE(std::find(names.begin(), names.end(), "Pre-EQ") != names.end());
    REQUIRE(std::find(names.begin(), names.end(), "Post-EQ") != names.end());

    // The "Post-EQ" instance selects "Pre-EQ" as its comparison source and reads
    // that instance's latest spectrum.
    SpectrumSource* source = reg.find("Pre-EQ");
    REQUIRE(source != nullptr);
    auto remote = source->latest_spectrum();
    REQUIRE(remote.version == 7);
    REQUIRE(remote.bins_db.front() == -6.0f);  // Pre-EQ's curve, not Post-EQ's
}

TEST_CASE("Destroying an instance cleans it from the registry", "[registry]") {
    InstanceRegistry reg;

    auto a = std::make_unique<RegisteredSpectrumSource>(
        "A", [] { return snap(1, -10.0f); }, reg);
    {
        auto b = std::make_unique<RegisteredSpectrumSource>(
            "B", [] { return snap(1, -12.0f); }, reg);
        REQUIRE(reg.size() == 2);
        REQUIRE(reg.find("B") != nullptr);
    }  // b destroyed here

    // Stale cleanup: B is gone, no dangling pointer; a selected-by-name remote
    // simply reads as unavailable (nullptr).
    REQUIRE(reg.size() == 1);
    REQUIRE(reg.find("B") == nullptr);
    REQUIRE(reg.find("A") != nullptr);
}

TEST_CASE("Selecting a never-present source is unavailable, not a crash",
          "[registry]") {
    InstanceRegistry reg;
    REQUIRE(reg.find("Nonexistent") == nullptr);
    REQUIRE(reg.names().empty());
}

TEST_CASE("register_source is idempotent", "[registry]") {
    InstanceRegistry reg;
    auto a = std::make_unique<RegisteredSpectrumSource>(
        "A", [] { return snap(1, 0.0f); }, reg);
    reg.register_source(a.get());  // double-register
    REQUIRE(reg.size() == 1);
}
