# Corpus

This directory tracks corpus metadata and app-owned future runtime metadata.

The source PDF corpus comes from Hanon-Online:

https://www.hanon-online.com/

Hanon-Online describes the downloadable sheets as Hanon educational materials
and states that users may promote Hanon exercises by printing and sharing the
PDFs.

The local mirror under `corpus/source-pdfs/` is intentionally not committed by
default. The source PDF manifest keeps public source URLs, expected local
relative paths, and byte counts so a local development checkout can recreate or
verify the private mirror without publishing the PDF files themselves.

Generated runtime score assets under `corpus/runtime/assets/` are also ignored
by default. They are source-derived build artifacts until a later release policy
decides otherwise.

Tracked corpus files currently include:

- `catalog/variants.tsv`: source-derived variant metadata; the only production
  row is `hanon-01-c` at `asset_only`
- `overlays/overlay_metadata.tsv`: header-only production overlay schema
- `overlays/overlay_steps.tsv`: header-only production overlay-step schema
- `manual-input/templates/`: header-only TSV templates for reviewed manual
  overlay and MIDI fixture entry

The source PDFs, score layouts, site images, and site text are third-party
source material and are not relicensed by this repository.
