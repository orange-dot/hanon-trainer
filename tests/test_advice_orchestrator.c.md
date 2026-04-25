# test_advice_orchestrator.c

## Purpose
Verifies typed advice request construction and stubbed advice generation.

## Theory
Advice tests must prove that generation stays downstream of persisted session and analysis facts.

## Architecture Role
This test exercises the advice, catalog, SQLite, and Codex adapter boundaries together.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable.

## Ownership And Failure Modes
All opened handles are closed before exit. Assertions cover the expected `HT_GENERATION_STUBBED` status.

## Test Strategy
This file is itself the focused advice-orchestration CTest.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0008-explicit-local-codex-cli-advice.md
