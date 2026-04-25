# ADR-0006: MIDI Ingress For Captured Takes

## Status

Accepted

## Context

The score-guidance-only posture is not enough for the intended teaching tool.

The app needs a way to observe what the user actually played so that it can
produce local review metrics and targeted advice.

## Decision

Add MIDI ingress for captured takes.

Capture is explicit:

- the user arms a device
- the user starts and stops a take
- `midi_capture` emits session-relative MIDI events
- the app/session orchestration layer persists captured MIDI for that session

The first build does not perform live coaching during active capture.

`midi_capture` must not depend on SQLite. It owns device enumeration and capture
mechanics only; persistence remains behind `sqlite_store`.

Captured event timestamps use nanoseconds relative to the start of the capture
session. Wall-clock session timestamps stay on the practice session record.

## Consequences

- the architecture gains a device-input boundary and session-lifecycle state
- the UI must expose capture readiness and capture failure states clearly
- analysis becomes possible for variants that have analysis-ready overlays
- fixture MIDI and live MIDI use the same session-relative timing basis
- the compile-time library graph stays acyclic because capture does not depend
  on persistence
- no microphone or audio path is required in the first build
