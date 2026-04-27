# test_platform_boundaries.c

## Purpose
Verifies SDL, guidance, and MIDI capture boundary behavior without display or hardware requirements.

## Theory
Platform integrations should compile and link early while runtime device/window behavior remains explicit and failure-aware.

## Architecture Role
This test guards UI-adjacent and MIDI platform leaves in the architecture graph.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable.

## Ownership And Failure Modes
All handles are closed/destroyed. MIDI start with an unavailable fixture id is expected to fail with not-found or backend I/O status.

## Test Strategy
It covers score renderer probe, guidance lifetime, MIDI list argument validation, empty poll semantics, and unavailable-device start behavior.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
