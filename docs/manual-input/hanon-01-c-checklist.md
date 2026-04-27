# Hanon 01 C Manual Input Checklist

Status: first bounded pilot passage entered.

This packet now records the first reviewed bounded passage for pilot analysis.
Future passes still need human review before additional real coordinates,
pitches, fingering labels, or timing expectations are copied into production
overlay TSV files.

## Source Asset

- Variant: `hanon-01-c`
- Source PDF: `corpus/source-pdfs/hanon-exercise-01-c.pdf`
- Runtime PPM: `corpus/runtime/assets/hanon-exercise-01-c.ppm`
- Expected runtime dimensions: `2480 x 3508`
- Current support level: `HT_SUPPORT_PILOT_ANALYSIS`
- Current overlay: `hanon-01-c-pilot-001`

## Required Human Review

- Confirm the generated PPM is visually aligned with the source PDF.
- Confirm the current first-measure rectangles against the generated PPM.
- Expand beyond the first bounded passage only after separate review.
- Record any new cursor rectangles against the generated PPM dimensions.
- Record any new expected MIDI pitches per step.
- Record timing windows only after deciding the reference tempo model.
- Decide whether finger labels belong in v1 or should remain empty.

## Files Populated For The Pilot Passage

- `corpus/overlays/overlay_metadata.tsv`
- `corpus/overlays/overlay_steps.tsv`
- `tests/fixtures/hanon-01-c/` regression TSV fixtures

## Guardrails

- Keep fake identifiers in `tests/fixtures/`.
- Do not promote `hanon-01-c` above `HT_SUPPORT_PILOT_ANALYSIS` until
  full-exercise reviewed overlay rows exist.
- Keep source PDFs ignored; only derived runtime assets and metadata policy are
  versioned.
