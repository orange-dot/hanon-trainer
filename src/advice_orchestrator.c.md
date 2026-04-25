# advice_orchestrator.c

## Purpose
Implements typed request assembly and stubbed advice artifact generation.

## Theory
The implementation keeps generated advice downstream of local metrics and catalog facts.

## Architecture Role
This source coordinates `sqlite_store`, `content_catalog`, and `codex_cli_adapter` without invoking external tools in the first slice.

## Implementation Contract
Request construction loads session, variant, and optional analysis data. Generation stores `HT_GENERATION_STUBBED` advice and returns `HT_OK`.

## Ownership And Failure Modes
All strings are copied into caller-owned transfer records. Missing session or catalog data propagates as status errors.

## Test Strategy
`tests/test_advice_orchestrator.c` checks request summaries and stubbed generation.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0008-explicit-local-codex-cli-advice.md
