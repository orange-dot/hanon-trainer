# score_renderer.h

## Purpose
Declares the score renderer lifetime and compile/link probe boundary.

## Theory
SDL integration is volatile and should remain isolated from domain parsing, persistence, and analysis.

## Architecture Role
This is a UI-adjacent platform boundary for mirrored score assets and future overlays.

## Implementation Contract
The first slice must not open a window or initialize SDL video. It only proves SDL2 and SDL_ttf linkability.

## Ownership And Failure Modes
The caller owns the renderer handle. Probe failures are status-code failures, not process aborts.

## Test Strategy
`tests/test_platform_boundaries.c` verifies create/probe/destroy without display requirements.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
