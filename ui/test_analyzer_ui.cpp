#include <catch2/catch_test_macros.hpp>

#include "analyzer_ui.hpp"
#include "demo_spectra.hpp"

#include <pulp/view/screenshot.hpp>
#include <pulp/view/screenshot_compare.hpp>

#include <fstream>
#include <string>
#include <vector>

using namespace pulp::view;
using pulp::view::SpectrumView;

namespace {

std::string dir() {
#ifdef ANALYZER_UI_DIR
    return ANALYZER_UI_DIR;
#else
    return ".";
#endif
}

std::vector<uint8_t> read_file(const std::string& p) {
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

std::vector<uint8_t> render_normal() {
    auto v = pulp::analyzer::ui::build_spectrum_panel(
        "Spectrum — 1 kHz tone", pulp::analyzer::demo::normal_bins(),
        SpectrumView::Style::bars, -120.0f, 0.0f);
    return render_to_png(*v, 480, 260, 1.0f, ScreenshotBackend::skia);
}

std::vector<uint8_t> render_diff() {
    auto v = pulp::analyzer::ui::build_spectrum_panel(
        "Diff — after vs before", pulp::analyzer::demo::diff_bins(),
        SpectrumView::Style::filled, -48.0f, 48.0f);
    return render_to_png(*v, 480, 260, 1.0f, ScreenshotBackend::skia);
}

void check_against_baseline(const std::vector<uint8_t>& png,
                            const std::string& name) {
    REQUIRE_FALSE(png.empty());
    auto stats = analyze_screenshot_content(png);
    REQUIRE(stats.passes_content_floor());
    auto baseline = read_file(dir() + "/" + name);
    if (baseline.empty()) {
        SKIP("no baseline " + name + " checked in (run the bake step)");
    }
    auto cmp = compare_screenshots(baseline, png);
    REQUIRE(cmp.valid);
    REQUIRE(cmp.similarity > 0.97);
}

} // namespace

TEST_CASE("analyzer normal spectrum view matches baseline", "[analyzer-ui]") {
    auto png = render_normal();
    if (png.empty()) SKIP("Skia raster backend unavailable");
    check_against_baseline(png, "normal-baseline.png");
}

TEST_CASE("analyzer diff spectrum view matches baseline", "[analyzer-ui]") {
    auto png = render_diff();
    if (png.empty()) SKIP("Skia raster backend unavailable");
    check_against_baseline(png, "diff-baseline.png");
}
