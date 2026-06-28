# Roadmap

The analyzer is built in layers so each lands with its own tests and the pure
parts never depend on GUI/IPC. Layer 1 exists today; the rest are sequenced here.

## Layer 1 — Analysis (done)

`engine/spectrum_engine.hpp` — windowed real-FFT → log-frequency magnitude (dB),
peak-per-band folding. Headless tests in `engine/test_spectrum_engine.cpp`.

Future refinement: a sharper constant-Q (sparse-kernel) analysis. The current
contract ("log-frequency magnitude in dB") stays stable.

## Layer 2 — Diff model ✅ (done)

`engine/spectrum_diff.hpp` — `diff_spectra(before, after, threshold)` returns a
per-band delta (dB) plus a boost/cut/flat classification, and rejects
empty/mismatched inputs. `ViewMode` (Normal / Diff / Both) models how the UI
presents it. Pure and headless-tested (`engine/test_spectrum_diff.cpp`).

Still to come on top of this: a stable reference capture and the source overlay
wiring (those belong with the registry + UI layers below).

## Layer 3 — Realtime publication ✅ (done)

`engine/rt_publisher.hpp` — `RtSpectrumPublisher`: the audio thread copies
samples into a bounded lock-free ring (`pulp::runtime::SpscQueue`, no alloc/lock/
FFT); a worker drains it, runs Layer 1 OFF the callback, and publishes the latest
spectrum via `pulp::runtime::TripleBuffer`; the UI reads the latest snapshot.
Ring overflow drops whole frames (counted), never partial/torn data. Headless
tests (`engine/test_rt_publisher.cpp`): spectrum-of-pushed-audio, overflow drops,
version advances.

**Hard RT rule held:** the FFT runs on the worker, never in `push_audio()`.

## Layer 4 — UI

GPU spectrogram / curve via Pulp's `SpectrumView` / `SpectrogramView` widgets,
with the diff overlay, hover readouts, and a legend. Screenshot baselines for
Normal and Diff states (Skia backend, content-floor checked).

## Cross-instance registry (on top of 1–3)

Lets a later instance select an earlier instance as its comparison source.

**In-process ✅ (done).** `engine/registry.hpp` — `InstanceRegistry` +
`SpectrumSource` / RAII `RegisteredSpectrumSource`: instances register by NAME
on construction and deregister on destruction (stale cleanup, no dangling
pointers); another instance selects a source by name and reads its latest
snapshot. Mutex-guarded control plane (never the audio thread). Headless tests
(`engine/test_registry.cpp`): two instances compare by name, destroy-cleans-up,
missing source reads unavailable, idempotent register.

**Scope decision (load-bearing):** the in-process registry is correct when the
host keeps instances in one address space. Sandboxed AU/VST3/CLAP instances can
land in **separate processes**, so true cross-process comparison needs
shared-memory IPC (`pulp::events::InterprocessConnection` / a shared-memory
mirror) — a deliberate follow-up, not a quick add. Standalone builds also need
`mute_preview_output`-style feedback avoidance so selecting a source can't create
a mic→speaker loop.

## Shared engine with the in-tree Audio Inspector

Layers 1–4 are the reusable analysis/diff/render stack. Pulp's in-tree Audio
Inspector dev tool can consume the same layers to add a before/after diff *view*
for its own probes — but under the Inspector's "FFT off the audio callback" rule,
that is additive work, not something already present. Keep the shared code in
neutral, dependency-light layers so the dev tool never depends on this plugin's
product code (registry, packaging).
