# test_platform_boundaries.c

## Purpose
Verifies SDL, guidance, and ALSA boundary behavior without display or hardware requirements.

## Theory
Platform integrations should compile and link early while runtime device/window behavior remains explicit and deferred.

## Architecture Role
This test guards UI-adjacent and MIDI platform leaves in the architecture graph.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable.

## Ownership And Failure Modes
All handles are closed/destroyed. MIDI start is expected to return unsupported in this slice.

## Test Strategy
It covers score renderer probe, guidance lifetime, and MIDI empty list/poll semantics.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0006-midi-ingress-for-captured-takes.md
