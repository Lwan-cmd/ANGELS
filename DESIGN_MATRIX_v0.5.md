# INGENIUM v0.5 — Internal Mod Matrix

Second page layout:

- Upper section: three LFO strips (RATE, DEPTH, SHAPE, TARGET).
- Lower section: four cross-modulation routes.
- Each route exposes SOURCE -> TARGET, MODE (CONTROL/AUDIO), and bipolar AMOUNT.
- Sources and targets are the six core modules only: AGE, MELT, GRAIN, GHOST, SMEAR, CHAOS.
- MIX remains output-only and is never a modulation source or target.

Audio mode uses a one-sample-delayed module tap as an audio-rate modulator. Control mode applies a low-pass/smoothing stage to the same tap before modulation. The one-sample delay intentionally breaks instantaneous feedback loops so self/circular routings remain bounded by the macro clamping.
