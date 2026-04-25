# test_overlay_store.c

## Purpose
Verifies overlay store empty, valid, and malformed input behavior.

## Theory
Production overlay files can exist as header-only schemas before manual input; malformed present data must fail.

## Architecture Role
This test guards the `overlay_store` boundary before analysis depends on it.

## Implementation Contract
The test uses `<assert.h>` and one `int main(void)` executable.

## Ownership And Failure Modes
Each opened store is closed. A failed open must leave the output handle NULL.

## Test Strategy
It covers production empty schema, synthetic valid steps, and bad metadata.

## Spec Links
- docs/LOCAL_SPEC.md
- docs/DATA_MODEL.md
- docs/CONTENT_PIPELINE.md
