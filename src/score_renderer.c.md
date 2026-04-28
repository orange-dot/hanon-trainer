# score_renderer.c

## Purpose
Implements the SDL2/SDL_ttf compile/link boundary and headless score snapshot
rendering.

## Theory
Rendering integrations should be isolated and testable without display side
effects. PPM parsing and SDL software surfaces provide a deterministic proof
before an interactive window exists.

## Architecture Role
This source is a UI-adjacent platform leaf for score asset display and active
overlay projection.

## Implementation Contract
The probe reads linked SDL versions only. The render path opens catalog and
overlay stores, resolves assets through `asset_root`, parses P6 PPM files,
projects overlay rectangles with deterministic integer math, renders to an SDL
software surface, and writes PPM snapshots without opening a window.

## Ownership And Failure Modes
The renderer handle is caller-owned. Missing catalog/overlay/asset content is
reported in result stage statuses when possible. Corrupt PPM data and output
write failures return non-OK statuses.

## Test Strategy
`tests/test_platform_boundaries.c` covers create/probe/destroy. Renderer tests
cover layout, PPM parsing, production Hanon projection, and CLI snapshots.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
