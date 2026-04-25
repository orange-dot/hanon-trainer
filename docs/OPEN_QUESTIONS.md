# Open Questions

This document remains prominent, but it no longer lists blockers for the first
implementation pass.

The core implementation decisions have been fixed. What remains are follow-up
questions for later iterations once the local C runtime, MIDI capture lane,
SQLite store, and Codex advice path are working.

## Resolved Decisions

| ID | Decision | Chosen Default |
| --- | --- | --- |
| `oq-1` | Canonical score display asset | Normalized image-first assets, with offline PDF-to-raster conversion if needed |
| `oq-2` | Overlay authoring | Manual-first sidecar overlays |
| `oq-3` | Corpus coverage on day one | Staged rollout with explicit `asset_only`, `guide_only`, `pilot_analysis`, and `analysis_ready` markers |
| `oq-4` | Persistence backend | SQLite from the first implementation pass |
| `oq-5` | Text rendering posture | `SDL2_ttf` is acceptable for the first build |
| `oq-6` | Hand-specific overlay detail | Coarse hand guidance, not per-note fingering evaluation |
| `oq-7` | Teaching mode | Post-session coaching only |
| `oq-8` | First analysis scope | Pitch and timing only |
| `oq-9` | AI integration boundary | Explicit local Codex CLI invocation through a typed advice request built from compact session facts |

## Remaining Follow-Up Questions

| ID | Question | Why It Matters | Current Posture |
| --- | --- | --- | --- |
| `fq-1` | When should fingering-aware scoring be added, if ever? | This changes overlay workload and analysis complexity. | Defer until pitch/timing scoring is trusted. |
| `fq-2` | Should the app eventually support phrase-by-phrase coaching between takes? | This affects session UX and capture-state transitions. | Keep the first build strictly post-session. |
| `fq-3` | Does the corpus eventually need richer annotation tooling for overlay maintenance? | This affects long-term authoring cost once coverage expands. | Start with manual-first files and revisit when authoring pain is real. |
| `fq-4` | Is audio capture worth adding after MIDI support stabilizes? | This would create a second analysis pipeline with different failure modes. | Keep audio out of scope for now. |

## Content Posture

The current posture still assumes private, local-only mirroring for personal
use.

That is acceptable for this implementation pack. Public redistribution remains
out of scope and unresolved.

## Implementation Readiness

No remaining item in this file should block a first implementation pass.

The current docs set is intended to be decision-complete for:

- C library boundaries
- local runtime configuration
- MIDI capture and persistence
- post-session pitch/timing analysis
- explicit local Codex advice generation through a typed request boundary
