# guidance_renderer.c

## Purpose
Implements the initial guidance renderer handle.

## Theory
The first vertical slice fixes lifetime and allocation behavior before rendering real guidance.

## Architecture Role
This source is a UI-adjacent leaf and does not import catalog, overlay, analysis, or SQLite concerns.

## Implementation Contract
Create allocates an opaque handle and destroy releases it. No rendering side effects occur yet.

## Ownership And Failure Modes
The caller owns the handle. Allocation failure returns `HT_ERR_IO`.

## Test Strategy
`tests/test_platform_boundaries.c` covers create/destroy behavior.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
