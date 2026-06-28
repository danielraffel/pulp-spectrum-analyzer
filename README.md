# Pulp Spectrum Analyzer

A **shippable spectrum-analyzer FX plugin**, built on the
[Pulp](https://www.generouscorp.com/pulp/) SDK, whose signature feature is a
**cross-instance before/after spectral diff**: drop one instance early in a host
chain and one later, select the earlier instance as the "source" in the later
one, and see what the chain did to the spectrum — boosts vs cuts — with the
source curve overlaid.

Unlike Pulp's built-in **Audio Inspector** developer tool (which observes Pulp's
*own* running plugin/standalone in the time domain), this is a normal insert that
works on **any** signal in a host chain, including third-party plugins, and adds
a true frequency-domain view. The two are complementary; see Pulp's
`docs/guides/audio-inspector.md` for the Inspector / Scope / Spectrum-Analyzer
distinction.

## Architecture (four independent layers)

The analyzer is deliberately layered so the pure parts are testable and the
GUI/IPC parts can't leak into them:

1. **Analysis** — log-frequency magnitude spectrum from a sample window. Pure,
   headless, deterministic. **Implemented + tested today** (`engine/`).
2. **Diff model** — boost/cut comparison of two spectra (Normal / Diff / Both),
   source overlay. _Planned._
3. **RT publication** — lock-free audio→UI handoff (`TripleBuffer`/`SpscQueue`);
   the **FFT runs off the audio callback**, never in `process()`. _Planned._
4. **UI** — GPU spectrogram / curve rendering. _Planned._

The cross-instance registry sits on top of layers 1–3.

## Status

| Piece | State |
|---|---|
| Layer 1 — `SpectrumEngine` (log-frequency magnitude in dB) | ✅ implemented, headless tests (`engine/test_spectrum_engine.cpp`) |
| Layer 2 — `SpectrumDiff` (before/after boost/cut classification) | ✅ implemented, headless tests (`engine/test_spectrum_diff.cpp`) |
| Layers 3–4 + plugin shell | 🚧 planned — see [ROADMAP.md](ROADMAP.md) |
| Cross-instance registry | 🚧 planned — in-process first; cross-process via IPC is a deliberate follow-up (see ROADMAP) |

## Attribution & clean-room policy

This is an **independent, clean-room** implementation. The DSP is standard
windowed-FFT magnitude analysis from the public signal-processing literature; the
cross-instance before/after comparison is a general, well-known analyzer idea.
No third-party analyzer source code was read, transcribed, or ported — the
implementation is built on Pulp's own primitives (`pulp::signal::Fft`, the
lock-free `TripleBuffer`/`SpscQueue`, and the `SpectrumView`/`SpectrogramView`
widgets). This mirrors
[Pulp's own licensing and clean-room discipline](https://www.generouscorp.com/pulp/licensing.html):
"we learned the shape; the implementation is independent."

## License

MIT — see [LICENSE](LICENSE). No third-party analyzer code is bundled.

## Building

Consumes the Pulp SDK (`find_package(Pulp)`); the simplest path is to scaffold
against a Pulp checkout/install with `pulp create`. Direct build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/path/to/pulp/install
cmake --build build -j
ctest --test-dir build --output-on-failure
```
