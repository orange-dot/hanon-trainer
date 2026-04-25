# ht_types.c

## Purpose
Implements common enum-to-string helpers and support-level parsing.

## Theory
Centralized conversion keeps on-disk tokens and UI/debug strings consistent across modules.

## Architecture Role
This source backs the root `ht_types.h` contract.

## Implementation Contract
Unknown enum values degrade to stable fallback strings. Support-level parsing accepts only documented TSV tokens.

## Ownership And Failure Modes
Returned strings are static storage. Invalid parse inputs return status codes and initialize output conservatively.

## Test Strategy
Catalog and advice tests exercise support-level parsing through loaded rows.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md
