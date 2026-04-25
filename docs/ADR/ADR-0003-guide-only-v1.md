# ADR-0003: Guide-Only V1

## Status

Superseded by ADR-0006 and ADR-0007

## Context

This ADR recorded the earlier viewer-and-guide posture that excluded MIDI
capture and scoring. That narrower direction is no longer the chosen first-build
target.

## Superseded Decision

The earlier decision kept the app guide-only and excluded MIDI input and scored
analysis.

## Replacement

The current architecture keeps the score-first posture but now includes:

- MIDI ingress for captured takes
- post-session pitch and timing analysis
- explicit local advice generation after analysis

## Consequences

- this ADR remains as a historical boundary marker
- implementation work should follow ADR-0006 and ADR-0007 instead
