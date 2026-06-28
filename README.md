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
| Persistent state — `InspectorState` (name / sources / view mode) | ✅ implemented, headless round-trip + corrupt-fallback tests (`engine/test_analyzer_state.cpp`) |
| Layer 3 — `RtSpectrumPublisher` (lock-free audio→worker→UI; FFT off the callback; ring overflow drops whole frames) | ✅ implemented, headless tests (`engine/test_rt_publisher.cpp`) |
| Layer 4 (UI slice) — spectrum panel, **two screenshot baselines** (normal + diff) | ✅ implemented, Skia raster tests (`ui/test_analyzer_ui.cpp`, `ui/{normal,diff}-baseline.png`) |
| Cross-instance registry (**in-process**: select a source by name, compare, stale cleanup) | ✅ implemented, headless tests (`engine/test_registry.cpp`) |
| Cross-**process** registry (shared-memory) + plugin shell | 🚧 planned — see [ROADMAP.md](ROADMAP.md) |
| Cross-instance registry | 🚧 planned — in-process first; cross-process via IPC is a deliberate follow-up (see ROADMAP) |

## Credits

Inspired by truce-audio's [truce-analyzer](https://github.com/truce-audio/truce-analyzer).

## License

MIT — see [LICENSE](LICENSE). See also [Pulp licensing](https://www.generouscorp.com/pulp/licensing.html).

## Building

Consumes the Pulp SDK (`find_package(Pulp)`); the simplest path is to scaffold
against a Pulp checkout/install with `pulp create`. Direct build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/path/to/pulp/install
cmake --build build -j
ctest --test-dir build --output-on-failure
```
