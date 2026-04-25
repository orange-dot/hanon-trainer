# c23_fallback.h

## Purpose
Provides a small compatibility point for C23-facing annotations while preserving the C11 baseline.

## Theory
Feature detection belongs behind a local fallback header so public headers do not directly depend on a compiler's newest dialect.

## Architecture Role
This internal header is optional support for the public C boundary and is intentionally leaf-like.

## Implementation Contract
Macros must degrade to valid C11 tokens and must not change ABI.

## Ownership And Failure Modes
No runtime ownership exists. The failure mode is compile-time incompatibility, covered by header independence tests.

## Test Strategy
`ht_header_independence` compiles this header as a standalone include.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md
