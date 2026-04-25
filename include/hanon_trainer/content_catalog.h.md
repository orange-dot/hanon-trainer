# content_catalog.h

## Purpose
Declares read-only lookup for source-derived exercise variants and mirrored score asset metadata.

## Theory
Catalog data is corpus-owned and immutable at runtime. The API returns plain records so callers can reason about identifiers and support levels without owning parser state.

## Architecture Role
This is a low-level domain boundary above `ht_types.h` and below session, analysis, and advice orchestration.

## Implementation Contract
Open/close follow the opaque handle convention. Lookups copy records into caller-owned storage.

## Ownership And Failure Modes
The catalog handle owns loaded TSV rows. Missing files are I/O errors; missing variants are `HT_ERR_NOT_FOUND`; malformed rows are corrupt data.

## Test Strategy
`tests/test_catalog.c` checks the real `hanon-01-c` asset-only row and synthetic fixture lookup.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/CONTENT_PIPELINE.md
