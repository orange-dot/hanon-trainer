# test_score_renderer_layout.c

## Purpose
Verifies deterministic score fit and overlay projection geometry.

## Theory
Viewer layout must be stable enough for repeatable pixel tests and later interactive navigation.

## Architecture Role
This test guards the public `score_renderer` render request/result contract.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable against the synthetic viewer corpus.

## Ownership And Failure Modes
The renderer handle is destroyed before exit. Invalid viewport and invalid step cases are expected status-code outcomes.

## Test Strategy
It checks square and letterboxed viewports, projected active rectangles, missing steps, and zero viewport rejection.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0001-sdl2-custom-ui.md
