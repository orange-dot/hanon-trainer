# codex_cli_adapter.c

## Purpose
Implements the local Codex CLI adapter handle.

## Theory
Configuration is copied into a handle so future process execution has explicit lifetime and timeout inputs.

## Architecture Role
This is a low-level integration source used by advice orchestration.

## Implementation Contract
Open validates executable path and timeout, copies optional model override, and returns an opaque handle.

## Ownership And Failure Modes
The caller owns the handle. Allocation or capacity failures are surfaced as `ht_status` values.

## Test Strategy
`tests/test_advice_orchestrator.c` opens and closes the adapter in the advice path.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ADR/ADR-0008-explicit-local-codex-cli-advice.md
