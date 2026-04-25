# ADR-0008: Explicit Local Codex CLI Advice

## Status

Accepted

## Context

The product now includes an AI-assisted teaching lane, but the app must stay
local-first and must not hide that assistance behind an opaque background
service.

The advice system also needs a bounded input contract so it does not absorb the
entire workspace or raw mirrored corpus by default.

## Decision

Integrate Codex through an explicit local CLI adapter.

Advice generation happens only when the user requests it from the review panel.

The public application boundary is a typed advice request, not an arbitrary raw
prompt string. The request is limited to:

- session and variant identity
- selected exercise metadata
- aggregate pitch and timing metrics
- weak-step summaries
- optional short user note

Any raw prompt text passed to the Codex CLI is an internal adapter detail derived
from that typed request.

## Consequences

- local analysis remains useful even when Codex is unavailable
- the AI boundary is explicit, auditable, and removable
- the UI must show Codex generation failures without hiding local metrics
- application code cannot accidentally attach the whole workspace, mirrored
  corpus, or hidden ambient context through a broad prompt API
- broader context attachment remains a future policy choice, not a silent
  default
