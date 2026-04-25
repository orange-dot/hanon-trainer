# Hanon 01 C Manual Input Checklist

Status: not started.

This packet stops before manual musical input. The next pass must be reviewed
by a human before any real coordinates, pitches, fingering labels, or timing
expectations are copied into production overlay TSV files.

## Source Asset

- Variant: `hanon-01-c`
- Source PDF: `corpus/source-pdfs/hanon-exercise-01-c.pdf`
- Runtime PPM: `corpus/runtime/assets/hanon-exercise-01-c.ppm`
- Expected runtime dimensions: `2480 x 3508`
- Current support level: `HT_SUPPORT_ASSET_ONLY`

## Required Human Review

- Confirm the generated PPM is visually aligned with the source PDF.
- Decide the first bounded passage for pilot analysis.
- Record cursor rectangles against the generated PPM dimensions.
- Record expected MIDI pitches per step.
- Record timing windows only after deciding the reference tempo model.
- Decide whether finger labels belong in v1 or should remain empty.

## Files To Populate After Review

- `corpus/overlays/overlay_metadata.tsv`
- `corpus/overlays/overlay_steps.tsv`
- optional fixture copy under `tests/fixtures/` for regression coverage

## Guardrails

- Keep fake identifiers in `tests/fixtures/`.
- Do not promote `hanon-01-c` above `HT_SUPPORT_ASSET_ONLY` until reviewed
  overlay rows exist.
- Keep source PDFs ignored; only derived runtime assets and metadata policy are
  versioned.
