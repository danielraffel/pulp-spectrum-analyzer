#pragma once

// InspectorState — persistent analyzer UI state (the plugin's custom,
// non-parameter state).
//
// The analyzer remembers a human-readable instance name, which remote sources
// the user selected to compare against (by NAME, not a transient runtime id),
// and the current view mode. Per the plan's state rules: unknown future fields
// are ignored, and malformed state falls back to defaults rather than throwing.
// This layer is pure and headless: a small versioned binary (de)serializer with
// no SDK dependency, so it can be unit-tested directly and later handed to
// Pulp's plugin_state_io as the processor's custom blob.

#include "spectrum_diff.hpp"  // ViewMode

#include <cstdint>
#include <string>
#include <vector>

namespace pulp::analyzer {

struct InspectorState {
    static constexpr uint32_t kMagic = 0x50535031;   // "PSP1"
    static constexpr uint32_t kSchemaVersion = 1;
    // Caps so a crafted blob can't drive an unbounded allocation.
    static constexpr uint32_t kMaxStringBytes = 64 * 1024;
    static constexpr uint32_t kMaxSources = 4096;

    std::string instance_name;
    std::vector<std::string> selected_sources;  // remote source names
    ViewMode view_mode = ViewMode::normal;

    // Versioned little-endian blob: magic, schema, name, source count + names,
    // view mode. Forward-compatible: a reader ignores trailing bytes it doesn't
    // understand (a newer writer may append fields), and a malformed blob yields
    // defaults via deserialize().
    std::vector<uint8_t> serialize() const {
        std::vector<uint8_t> out;
        put_u32(out, kMagic);
        put_u32(out, kSchemaVersion);
        put_str(out, instance_name);
        put_u32(out, static_cast<uint32_t>(selected_sources.size()));
        for (const auto& s : selected_sources) put_str(out, s);
        put_u32(out, static_cast<uint32_t>(view_mode));
        return out;
    }

    // Returns defaults on any malformed/short/foreign input — never throws
    // (the try/catch backstops even a pathological std::bad_alloc).
    static InspectorState deserialize(const std::vector<uint8_t>& in) {
        try {
            InspectorState s;
            std::size_t p = 0;
            uint32_t magic = 0, schema = 0;
            if (!get_u32(in, p, magic) || magic != kMagic) return InspectorState{};
            if (!get_u32(in, p, schema)) return InspectorState{};
            // Future schema bumps are tolerated for the fields we know; unknown
            // trailing data is ignored by simply not reading it.
            if (!get_str(in, p, s.instance_name)) return InspectorState{};
            uint32_t count = 0;
            if (!get_u32(in, p, count)) return InspectorState{};
            // Each source needs at least its 4-byte length prefix, so a count
            // that can't fit in the remaining bytes (or exceeds the cap) is
            // corrupt — reject before reserving/looping.
            if (count > kMaxSources || count > (in.size() - p) / 4) {
                return InspectorState{};
            }
            s.selected_sources.reserve(count);
            for (uint32_t i = 0; i < count; ++i) {
                std::string name;
                if (!get_str(in, p, name)) return InspectorState{};
                s.selected_sources.push_back(std::move(name));
            }
            uint32_t mode = 0;
            if (!get_u32(in, p, mode)) return InspectorState{};
            s.view_mode = (mode <= static_cast<uint32_t>(ViewMode::both))
                              ? static_cast<ViewMode>(mode)
                              : ViewMode::normal;
            return s;
        } catch (...) {
            return InspectorState{};
        }
    }

private:
    static void put_u32(std::vector<uint8_t>& o, uint32_t v) {
        o.push_back(static_cast<uint8_t>(v & 0xFF));
        o.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        o.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        o.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    }
    static void put_str(std::vector<uint8_t>& o, const std::string& s) {
        put_u32(o, static_cast<uint32_t>(s.size()));
        o.insert(o.end(), s.begin(), s.end());
    }
    // Subtraction-after-bounds idiom (never overflow p + needed on untrusted len).
    static bool get_u32(const std::vector<uint8_t>& in, std::size_t& p, uint32_t& v) {
        if (p > in.size() || in.size() - p < 4) return false;
        v = static_cast<uint32_t>(in[p]) |
            (static_cast<uint32_t>(in[p + 1]) << 8) |
            (static_cast<uint32_t>(in[p + 2]) << 16) |
            (static_cast<uint32_t>(in[p + 3]) << 24);
        p += 4;
        return true;
    }
    static bool get_str(const std::vector<uint8_t>& in, std::size_t& p, std::string& s) {
        uint32_t len = 0;
        if (!get_u32(in, p, len)) return false;
        if (len > kMaxStringBytes) return false;            // cap allocation
        if (p > in.size() || in.size() - p < len) return false;
        s.assign(in.begin() + p, in.begin() + p + len);
        p += len;
        return true;
    }
};

} // namespace pulp::analyzer
