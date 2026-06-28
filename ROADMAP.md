# Roadmap

The analyzer is built in layers so each lands with its own tests and the pure
parts never depend on GUI/IPC. Layer 1 exists today; the rest are sequenced here.

## Layer 1 — Analysis (done)

`engine/spectrum_engine.hpp` — windowed real-FFT → log-frequency magnitude (dB),
peak-per-band folding. Headless tests in `engine/test_spectrum_engine.cpp`.

Future refinement: a sharper constant-Q (sparse-kernel) analysis. The current
contract ("log-frequency magnitude in dB") stays stable.

## Layer 2 — Diff model

Compare two spectra into a boost/cut curve with a stable reference, plus
Normal / Diff / Both view modes and a source overlay. Pure and headless-testable
(deterministic input spectra → expected diff). No GUI dependency.

## Layer 3 — Realtime publication

Audio thread taps a bounded sample window into a lock-free ring
(`pulp::runtime::SpscQueue`), a background worker drains it and runs Layer 1, and
the latest spectrum is published via `pulp::runtime::TripleBuffer` for the UI.

**Hard RT rule:** the FFT runs on the worker, never in `process()`. `process()`
does no allocation, no locks, no blocking; ring overflow drops whole frames.

## Layer 4 — UI

GPU spectrogram / curve via Pulp's `SpectrumView` / `SpectrogramView` widgets,
with the diff overlay, hover readouts, and a legend. Screenshot baselines for
Normal and Diff states (Skia backend, content-floor checked).

## Cross-instance registry (on top of 1–3)

Lets a later instance select an earlier instance as its comparison source.

**Scope decision (load-bearing):** start **in-process** — a process-local
registry of live instances, which is correct when the host keeps instances in one
address space. AU/VST3/CLAP instances can be sandboxed into **separate
processes**, so true cross-process comparison needs shared-memory IPC
(`pulp::events::InterprocessConnection` / a shared-memory mirror). That is a
deliberate follow-up, not a quick add; the in-process version ships first and
documents the limitation. Standalone builds need `mute_preview_output`-style
feedback avoidance so selecting a source can't create a mic→speaker loop.

## Shared engine with the in-tree Audio Inspector

Layers 1–4 are the reusable analysis/diff/render stack. Pulp's in-tree Audio
Inspector dev tool can consume the same layers to add a before/after diff *view*
for its own probes — but under the Inspector's "FFT off the audio callback" rule,
that is additive work, not something already present. Keep the shared code in
neutral, dependency-light layers so the dev tool never depends on this plugin's
product code (registry, packaging).
