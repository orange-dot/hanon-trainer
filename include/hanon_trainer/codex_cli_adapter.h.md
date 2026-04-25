# codex_cli_adapter.h

## Purpose
Declares the local Codex CLI adapter handle and lifetime functions.

## Theory
The CLI is an external process boundary. A C library should expose a narrow typed handle rather than raw prompt execution throughout the app.

## Architecture Role
This header is a lower-level integration boundary consumed by `advice_orchestrator.h`.

## Implementation Contract
`ht_codex_cli_open` copies borrowed configuration strings into the handle. `ht_codex_cli_close` accepts NULL.

## Ownership And Failure Modes
The caller owns the handle returned through `out_cli`. Invalid paths or timeout values are rejected before any external process is run.

## Test Strategy
`tests/test_advice_orchestrator.c` opens and closes a stub adapter as part of advice generation.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0008-explicit-local-codex-cli-advice.md
