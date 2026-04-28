# test_hanon_01c_viewer_projection.c

## Purpose
Verifies production Hanon 01C pilot overlay projection without requiring the production PPM asset.

## Theory
The production catalog and overlay rows should already be sufficient to compute deterministic viewport geometry for step navigation.

## Architecture Role
This test connects the production corpus metadata to the `score_renderer` projection result.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable. It leaves `output_path` empty so no score asset load or window is required.

## Ownership And Failure Modes
The renderer handle is destroyed before exit. Invalid production step indexes are expected to produce not-found result statuses.

## Test Strategy
It projects all eight `hanon-01-c-pilot-001` steps into a 1280x900 viewport and checks left-to-right ordering.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
