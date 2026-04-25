# ADR-0004: Local-Only Offline Posture

## Status

Accepted

## Context

The intended use is private personal practice on one Linux machine. The product
is not a public content platform or sync-heavy service.

The expanded first build adds MIDI capture and Codex assistance, but both still
need to preserve a fully local posture.

## Decision

Keep the app offline-first and local-only.

Treat mirrored content as private local material and keep public redistribution
out of scope.

Use the local Codex CLI only as an explicit on-device tool invocation.

## Consequences

- no account system is needed
- no remote sync or cloud state is needed
- captured sessions, analysis output, and advice artifacts stay on the local
  machine
- the Codex integration must not hide a remote service assumption inside the
  runtime model
- any future public distribution plan still requires a separate policy decision
