# overlay_store.c

## Purpose
Implements TSV-backed overlay metadata and step loading.

## Theory
Overlay files model reviewed coordinates and pitch/timing expectations separately from static score assets.

## Architecture Role
This source backs the `overlay_store.h` domain boundary and feeds analysis.

## Implementation Contract
It accepts missing overlay TSVs as an empty store, validates present rows, parses expected pitch lists, and returns copied records.

## Ownership And Failure Modes
The store owns loaded overlay and step arrays. Malformed rows return corrupt-data or buffer errors.

## Test Strategy
`tests/test_overlay_store.c` covers empty production schema, valid fixture data, and malformed input.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/CONTENT_PIPELINE.md
