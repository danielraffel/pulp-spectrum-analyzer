#include <catch2/catch_test_macros.hpp>

#include "analyzer_state.hpp"

using namespace pulp::analyzer;

TEST_CASE("InspectorState round-trips name, sources, and view mode", "[state]") {
    InspectorState s;
    s.instance_name = "Master Bus After";
    s.selected_sources = {"Pre-EQ", "Drum Bus"};
    s.view_mode = ViewMode::diff;

    auto restored = InspectorState::deserialize(s.serialize());
    REQUIRE(restored.instance_name == "Master Bus After");
    REQUIRE(restored.selected_sources == std::vector<std::string>{"Pre-EQ", "Drum Bus"});
    REQUIRE(restored.view_mode == ViewMode::diff);
}

TEST_CASE("InspectorState defaults round-trip", "[state]") {
    InspectorState s;  // empty name, no sources, normal mode
    auto restored = InspectorState::deserialize(s.serialize());
    REQUIRE(restored.instance_name.empty());
    REQUIRE(restored.selected_sources.empty());
    REQUIRE(restored.view_mode == ViewMode::normal);
}

TEST_CASE("InspectorState falls back to defaults on malformed input", "[state]") {
    // Empty, truncated, and foreign-magic blobs all yield defaults, never throw.
    REQUIRE(InspectorState::deserialize({}).instance_name.empty());
    REQUIRE(InspectorState::deserialize({0x01, 0x02, 0x03}).selected_sources.empty());

    std::vector<uint8_t> foreign{0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x00, 0x00, 0x00};
    auto r = InspectorState::deserialize(foreign);
    REQUIRE(r.instance_name.empty());
    REQUIRE(r.view_mode == ViewMode::normal);

    // A blob that claims an absurd source count must not over-read.
    std::vector<uint8_t> bad = InspectorState{}.serialize();
    bad[8 + 4] = 0xFF;  // corrupt the name-length / count region
    auto r2 = InspectorState::deserialize(bad);
    REQUIRE(r2.view_mode == ViewMode::normal);  // safe defaults
}

TEST_CASE("InspectorState is hardened against crafted blobs", "[state]") {
    using S = InspectorState;
    // magic "PSP1" little-endian = 0x31 0x50 0x53 0x50.

    // Oversized declared string length (4 GiB) with no payload → defaults.
    std::vector<uint8_t> oversized{0x31, 0x50, 0x53, 0x50, 1, 0, 0, 0,
                                   0xFF, 0xFF, 0xFF, 0xFF};
    REQUIRE_NOTHROW(S::deserialize(oversized));
    REQUIRE(S::deserialize(oversized).instance_name.empty());

    // Truncated string payload (claims 10 bytes, supplies 2) → defaults.
    std::vector<uint8_t> truncated{0x31, 0x50, 0x53, 0x50, 1, 0, 0, 0,
                                   10, 0, 0, 0, 'a', 'b'};
    REQUIRE(S::deserialize(truncated).instance_name.empty());

    // Absurd source count (1,000,000) → rejected, no huge allocation, no throw.
    std::vector<uint8_t> bigcount{0x31, 0x50, 0x53, 0x50, 1, 0, 0, 0,
                                  0, 0, 0, 0, 0x40, 0x42, 0x0F, 0x00};
    REQUIRE_NOTHROW(S::deserialize(bigcount));
    REQUIRE(S::deserialize(bigcount).selected_sources.empty());

    // Invalid ViewMode value normalizes to normal.
    S s;
    s.instance_name = "x";
    s.view_mode = ViewMode::diff;
    auto blob = s.serialize();
    blob[blob.size() - 4] = 99;  // corrupt the mode field
    REQUIRE(S::deserialize(blob).view_mode == ViewMode::normal);
}

TEST_CASE("InspectorState tolerates trailing unknown bytes (forward compat)",
          "[state]") {
    InspectorState s;
    s.instance_name = "Inst";
    s.view_mode = ViewMode::both;
    auto blob = s.serialize();
    blob.push_back(0xAB);  // a newer writer appended a field we don't know
    blob.push_back(0xCD);
    auto restored = InspectorState::deserialize(blob);
    REQUIRE(restored.instance_name == "Inst");
    REQUIRE(restored.view_mode == ViewMode::both);
}
