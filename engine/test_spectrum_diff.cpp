#include <catch2/catch_test_macros.hpp>

#include "spectrum_diff.hpp"

using namespace pulp::analyzer;

TEST_CASE("diff_spectra classifies boost, cut, and flat bands", "[diff]") {
    std::vector<float> before{-30.0f, -30.0f, -30.0f, -30.0f};
    std::vector<float> after{-30.0f, -24.0f, -36.0f, -30.5f};
    auto r = diff_spectra(before, after, 1.0f);

    REQUIRE(r.valid);
    REQUIRE(r.delta_db.size() == 4);
    REQUIRE(r.classification[0] == BandChange::flat);   // 0 dB
    REQUIRE(r.classification[1] == BandChange::boost);  // +6 dB
    REQUIRE(r.classification[2] == BandChange::cut);    // -6 dB
    REQUIRE(r.classification[3] == BandChange::flat);   // -0.5 dB within threshold
    REQUIRE(r.delta_db[1] == 6.0f);
    REQUIRE(r.delta_db[2] == -6.0f);
}

TEST_CASE("diff_spectra honors the threshold", "[diff]") {
    std::vector<float> before{0.0f};
    std::vector<float> after{2.0f};
    REQUIRE(diff_spectra(before, after, 1.0f).classification[0] == BandChange::boost);
    REQUIRE(diff_spectra(before, after, 3.0f).classification[0] == BandChange::flat);
}

TEST_CASE("diff_spectra rejects mismatched or empty input", "[diff]") {
    REQUIRE_FALSE(diff_spectra({}, {}).valid);
    REQUIRE_FALSE(diff_spectra({1.0f, 2.0f}, {1.0f}).valid);
    REQUIRE(diff_spectra({1.0f}, {1.0f}).valid);
}
