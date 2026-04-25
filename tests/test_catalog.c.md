# test_catalog.c

## Purpose
Verifies production and synthetic catalog lookup behavior.

## Theory
The first real row must be asset-only, while fake IDs remain in test fixtures.

## Architecture Role
This test guards the `content_catalog` boundary and the corpus catalog schema.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable.

## Ownership And Failure Modes
The catalog handle is closed after each corpus root. Missing lookup is expected to return `HT_ERR_NOT_FOUND`.

## Test Strategy
It checks `hanon-01-c` dimensions/support level and synthetic overlay linkage.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/CONTENT_PIPELINE.md
