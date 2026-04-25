# advice_orchestrator.h

## Purpose
Defines the typed advice orchestration boundary between persisted local facts, the catalog, and the local Codex CLI adapter.

## Theory
Advice is derived from already-recorded evidence. The API accepts handles and transfer records so generation cannot mutate analysis truth or raw capture data.

## Architecture Role
This header sits above `sqlite_store.h`, `content_catalog.h`, and `codex_cli_adapter.h` in the documented graph. It owns coordination, not storage or process details.

## Implementation Contract
Functions return `ht_status`. Output records are caller-owned and initialized on `HT_OK`; failures zero output where documented.

## Ownership And Failure Modes
All handles are borrowed. Missing session/catalog rows return lookup errors. The first slice intentionally reports generated advice as `HT_GENERATION_STUBBED`.

## Test Strategy
`tests/test_advice_orchestrator.c` covers request construction and stub generation persistence.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ARCHITECTURE.md
- docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md
