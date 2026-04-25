# hanon-trainer

Documentation-only architecture and local implementation spec pack for a local
Linux-native Hanon practice and teaching app in `C`.

## Status

This repo currently contains no implementation code.

The pack now fixes the first-build direction for a local-first application with
MIDI capture, post-session teaching analysis, and explicit local Codex CLI
assistance. The remaining questions are non-blocking follow-up items rather
than core architecture gaps.

## Corpus Publication Posture

The source PDFs are mirrored from [Hanon-Online](https://www.hanon-online.com/).
The Hanon-Online exercise pages describe the downloadable sheets as Hanon
educational materials and state that users may promote Hanon exercises by
printing and sharing the PDFs.

The public repository tracks corpus metadata and attribution, not the mirrored
PDF files or generated runtime score assets. Local source PDFs remain under
`corpus/source-pdfs/` and are ignored by default.

See [Corpus](corpus/README.md), [Third-Party Notices](THIRD_PARTY_NOTICES.md),
and [License](LICENSE).

## License

Original hanon-trainer code, documentation, metadata, and tooling are licensed
under the MIT License.

Hanon-Online PDFs, sheet-music layouts, site text, and images remain
third-party source material. They are attributed in this repository but are not
relicensed by the MIT License.

## Product Posture

- local-only desktop app for personal practice
- Linux-native implementation in `C`
- score-first experience with keyboard guidance
- MIDI ingress for captured practice takes
- post-session pitch and timing analysis
- explicit local Codex CLI assistance for review summaries and practice advice
- no hidden cloud dependency
- no audio playback engine in the first build
- no microphone capture in the first build

## First-Build Shape

The current architecture target assumes:

- `SDL2` custom UI and rendering
- normalized image-first score assets with app-owned overlays
- C ABI-style public headers with `ht_` identifiers, opaque handles,
  `ht_status` returns, and caller-owned records or buffers
- one public header per semantically connected C library
- explicit translation-unit boundaries for runtime services
- SQLite as the canonical local store
- browser rail, score canvas, guidance area, practice sidebar, and review panel
- a manual `Generate Advice` action that invokes local Codex only after a
  completed take

## Review And Spec Documents

- [Architecture](docs/ARCHITECTURE.md)
- [Local Spec](docs/LOCAL_SPEC.md)
- [UI Concept](docs/UI_CONCEPT.md)
- [Content Pipeline](docs/CONTENT_PIPELINE.md)
- [Data Model](docs/DATA_MODEL.md)
- [Open Questions](docs/OPEN_QUESTIONS.md)
- [Sprint 001: Pilot Analysis Documentation Plan](docs/SPRINT-001-PILOT-TRAINER.md)
- [Review: Sprint 001](docs/REVIEW-SPRINT-001.md)
- [Backlog Refinement](docs/BACKLOG-REFINEMENT.md)
- [ADR-0001: SDL2 Custom UI](docs/ADR/ADR-0001-sdl2-custom-ui.md)
- [ADR-0002: Mirrored Image-First Score Assets](docs/ADR/ADR-0002-mirrored-score-assets.md)
- [ADR-0003: Guide-Only V1](docs/ADR/ADR-0003-guide-only-v1.md)
- [ADR-0004: Local-Only Offline Posture](docs/ADR/ADR-0004-local-only-offline-posture.md)
- [ADR-0005: SQLite Local Store](docs/ADR/ADR-0005-sqlite-local-store.md)
- [ADR-0006: MIDI Ingress For Captured Takes](docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md)
- [ADR-0007: Post-Session Pitch And Timing Analysis](docs/ADR/ADR-0007-post-session-pitch-and-timing-analysis.md)
- [ADR-0008: Explicit Local Codex CLI Advice](docs/ADR/ADR-0008-explicit-local-codex-cli-advice.md)
- [ADR-0009: C Library Boundaries And Interface Discipline](docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md)

## What This Pack Settles

This pack now chooses:

- image-first runtime score assets
- manual-first overlay authoring
- staged corpus coverage with explicit support markers
- SQLite now, not JSON-first persistence
- post-session coaching, not live scoring
- pitch and timing analysis in the first build
- explicit local Codex invocation with a compact session summary and exercise
  metadata

## Non-Goals

These items remain intentionally outside the first build:

- live chat-style coaching during active capture
- microphone or audio waveform analysis
- real-time adaptive scoring while the user is playing
- public content redistribution
- engraving-grade notation rendering
- remote sync or account-backed collaboration
