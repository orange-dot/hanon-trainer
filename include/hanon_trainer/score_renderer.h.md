# score_renderer.h

## Purpose
Declares the score renderer lifetime, compile/link probe boundary, and
headless snapshot render contract.

## Theory
SDL integration is volatile and should remain isolated from domain parsing,
persistence, and analysis. Public callers pass plain records; SDL surfaces stay
private to the implementation.

## Architecture Role
This is a UI-adjacent platform boundary for mirrored score assets and active
overlay projection.

## Implementation Contract
The render API keeps corpus and asset roots separate, resolves catalog assets,
projects one active overlay rectangle, and optionally writes a PPM snapshot. It
must not expose SDL types in the public header.

## Ownership And Failure Modes
The caller owns the renderer handle and request/result records. Missing content
is reported through result stage statuses; malformed assets or failed writes
return status-code failures.

## Test Strategy
`tests/test_platform_boundaries.c` verifies create/probe/destroy without
display requirements. Viewer tests cover layout, PPM loading, production Hanon
projection, and CLI snapshot behavior.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
