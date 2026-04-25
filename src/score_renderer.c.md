# score_renderer.c

## Purpose
Implements the SDL2 and SDL_ttf compile/link boundary for score rendering.

## Theory
Rendering integrations should be isolated and testable without display side effects.

## Architecture Role
This source is a UI-adjacent platform leaf for future score asset display.

## Implementation Contract
The probe reads linked SDL versions only. It must not call `SDL_Init(SDL_INIT_VIDEO)` or open a window.

## Ownership And Failure Modes
The renderer handle is caller-owned. Missing linked TTF metadata returns unsupported.

## Test Strategy
`tests/test_platform_boundaries.c` covers create/probe/destroy.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
