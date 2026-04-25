# guidance_renderer.h

## Purpose
Declares the guidance renderer lifetime boundary for future keyboard and cue presentation.

## Theory
Rendering state is mutable and platform-adjacent, so it is hidden behind an opaque handle while data remains in transfer records.

## Architecture Role
This header is a UI-adjacent leaf boundary. It does not own persistence, corpus, MIDI capture, or analysis state.

## Implementation Contract
Create/destroy follow the handle convention. The first slice only proves lifetime shape.

## Ownership And Failure Modes
The caller owns the returned renderer. Allocation failure returns `HT_ERR_IO`; destroy accepts NULL.

## Test Strategy
`tests/test_platform_boundaries.c` creates and destroys the renderer.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/UI_CONCEPT.md
