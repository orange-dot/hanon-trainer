# overlay_store.h

## Purpose
Declares read-only overlay metadata and step expectation lookup.

## Theory
Overlay data separates reviewed guidance and scoring expectations from source-derived catalog assets.

## Architecture Role
This domain boundary feeds session guidance and performance analysis while remaining independent of SQLite and rendering.

## Implementation Contract
Missing production overlay TSV files produce an empty store. Malformed present TSV rows produce corrupt-data failures.

## Ownership And Failure Modes
The store owns parsed rows. Caller-owned output records are initialized on success and zeroed on failures.

## Test Strategy
`tests/test_overlay_store.c` covers empty production overlays, valid synthetic overlays, and malformed TSV rejection.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/CONTENT_PIPELINE.md
