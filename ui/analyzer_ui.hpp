#pragma once

// analyzer_ui — minimal scripted-free View tree for the analyzer's spectrum
// display (layer 4, headless-renderable slice).
//
// Builds a titled panel containing a pulp::view::SpectrumView populated with a
// dB spectrum, laid out with Yoga flex so render_to_png(ScreenshotBackend::skia)
// produces a deterministic artifact. This is the screenshot-testable core of the
// UI; the live GPU spectrogram + interactive selectors build on the same widget.

#include <pulp/view/view.hpp>
#include <pulp/view/widgets.hpp>

#include <memory>
#include <string>
#include <vector>

namespace pulp::analyzer::ui {

inline std::unique_ptr<pulp::view::View> build_spectrum_panel(
    const std::string& title,
    const std::vector<float>& bins_db,
    pulp::view::SpectrumView::Style style,
    float min_db, float max_db) {
    using namespace pulp::view;
    using canvas::Color;

    auto root = std::make_unique<View>();
    root->set_background_color(Color::rgba8(20, 21, 26, 255));
    {
        auto& f = root->flex();
        f.direction = FlexDirection::column;
        f.align_items = FlexAlign::stretch;
        f.preferred_width = 480;
        f.preferred_height = 260;
        f.padding = 14;
        f.gap = 10;
    }

    auto label = std::make_unique<Label>(title);
    label->set_font_size(18.0f);
    label->set_text_color(Color::rgba8(232, 232, 238, 255));
    label->flex().preferred_height = 24;
    root->add_child(std::move(label));

    auto spectrum = std::make_unique<SpectrumView>();
    spectrum->set_spectrum(bins_db);
    spectrum->set_style(style);
    spectrum->set_range(min_db, max_db);
    spectrum->set_background_color(Color::rgba8(12, 13, 17, 255));
    {
        auto& f = spectrum->flex();
        f.flex_grow = 1;          // fill the remaining height
        f.preferred_height = 190;
    }
    root->add_child(std::move(spectrum));

    return root;
}

} // namespace pulp::analyzer::ui
