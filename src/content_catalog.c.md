# content_catalog.c

## Purpose
Implements TSV-backed catalog loading and variant lookup.

## Theory
The parser treats corpus metadata as immutable source-derived input and copies it into stable C transfer records.

## Architecture Role
This source backs the `content_catalog.h` domain boundary.

## Implementation Contract
It loads `catalog/variants.tsv`, validates fixed field counts, parses support levels, and stores rows in module-owned memory.

## Ownership And Failure Modes
The catalog owns its row array. Missing files return I/O errors; malformed rows return corrupt-data or buffer errors.

## Test Strategy
`tests/test_catalog.c` covers production and synthetic fixture rows.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/CONTENT_PIPELINE.md
