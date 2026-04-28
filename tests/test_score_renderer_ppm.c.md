# test_score_renderer_ppm.c

## Purpose
Verifies PPM loading, rendering, output writing, and malformed asset failures.

## Theory
The viewer pilot depends on deterministic P6 PPM assets and should reject malformed inputs before an interactive UI exists.

## Architecture Role
This test guards the private PPM/render path through the public `score_renderer` API.

## Implementation Contract
The executable accepts one case name argument so CTest can report the exact PPM scenario that fails or hangs. Each PPM case has a short CTest timeout to keep CI actionable on platforms where the failure reproduces only remotely. It writes snapshot outputs only in the build working directory.

## Ownership And Failure Modes
The renderer handle is destroyed before exit. Missing assets are degraded result statuses; malformed assets return non-OK statuses.

## Test Strategy
Separate CTest entries check a valid synthetic render, red overlay pixels, missing assets, bad magic, bad max value, short payload, malformed header, and overlay dimension mismatch.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
