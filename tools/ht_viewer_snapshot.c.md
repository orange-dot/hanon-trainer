# ht_viewer_snapshot.c

## Purpose
Provides a small CLI for rendering one headless score snapshot with an active overlay.

## Theory
The first viewer proof should be runnable in CI and local terminals without opening a desktop window.

## Architecture Role
This tool is a thin command-line adapter around `score_renderer`.

## Implementation Contract
The command parses required flags, calls `ht_score_renderer_render_view`, prints compact diagnostics, and maps stage statuses to stable process exit codes.

## Ownership And Failure Modes
The tool owns the renderer handle during one invocation. Invalid CLI input, missing assets, missing overlays, corrupt assets, and output failures produce distinct non-zero exits.

## Test Strategy
`tests/tools/check_viewer_snapshot_cli.py` exercises successful synthetic rendering and missing-asset handling.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
