# Content Pipeline

The app depends on a local mirrored Hanon corpus plus app-owned metadata that
makes score guidance and post-session analysis possible.

The source posture remains private and offline-first. The app is not a public
distribution vehicle for the mirrored material.

## Source Reality

The source PDF corpus is mirrored from Hanon-Online:

```text
https://www.hanon-online.com/
```

Hanon-Online identifies the downloadable sheets as Hanon educational materials
and permits promotion of Hanon exercises by printing and sharing the PDFs.

The mirrored source material exposes, per exercise page:

- score imagery
- downloadable PDF
- tempo and repetition text
- practice notes
- exercise and key structure

That is enough to build a score-first teaching tool without inventing a
notation engine.

## Offline Pipeline Stages

### 1. Mirror raw source material

Store the raw page content and linked score assets locally for personal use and
local development.

This preserves provenance and keeps the implementation honest about where the
display and teaching context originated.

The mirror should preserve Hanon-Online attribution and source URLs. Mirrored
PDFs, score layouts, page images, and site text remain third-party source
material; the app's original code, metadata, overlays, and tooling remain
separate project assets.

### 2. Normalize the variant catalog

Extract a stable local catalog for each exercise and key variant containing:

- stable `exercise_id` and `variant_id`
- exercise number and human-facing title
- key label
- source tempo and repetition text
- source practice notes
- local paths to mirrored page and source PDF
- support level for the current build
- display asset dimensions when a normalized image asset exists

### 3. Choose the canonical display asset

Each exercise and key pair gets one canonical runtime display asset.

Chosen default:

- use a normalized image-first asset derived from the mirrored score image or
  an offline PDF-to-raster step

This keeps runtime rendering simple and avoids PDF rendering as a hard runtime
dependency.

For Sprint 001, the Exercise 01 in C pilot pins page 1 at 300 DPI with
`pdftoppm`, yielding a `2480 x 3508` coordinate basis. Overlay metadata must
record the reference dimensions it was authored against, and the loader must
reject overlays whose reference dimensions do not match the display asset.

### 4. Author overlay sidecars

Each guided variant gets a sidecar overlay description for:

- score regions
- step order
- hand scope
- keyboard targets
- optional finger labels
- expected pitch groups per step
- timing basis or tolerance windows for first-pass analysis

Overlay authoring remains manual-first. Tooling can be added later, but the
first build assumes reviewable hand-authored sidecars.

### 5. Validate analysis readiness

Before a variant can be labeled `analysis_ready`, the pipeline must verify full
exercise coverage:

- one canonical display asset exists
- one overlay sidecar exists
- every overlay step maps to a valid page and score region
- every analysis-enabled step declares expected pitch content
- timing expectations are present for any step that will be scored
- the overlay covers the full scored exercise, not only a bounded passage

Variants that do not meet that bar remain visible but are labeled
`asset_only`, `guide_only`, or `pilot_analysis`.

`pilot_analysis` is reserved for bounded, manually reviewed passages whose
fixture-based analysis is valid but whose variant does not yet have full
exercise coverage. It must not be treated as equivalent to `analysis_ready`.

### 6. Package the runtime corpus

The runtime corpus is the combination of:

- mirrored source assets
- normalized variant catalog
- canonical display assets
- overlay metadata and step expectations

The runtime corpus remains read-only once packaged.

## Runtime Separation

The runtime writes nothing back into the mirrored corpus.

- source-derived assets stay in the corpus tree
- overlay sidecars stay in the corpus tree
- user state, sessions, MIDI events, analysis output, and advice artifacts live
  in SQLite

This keeps offline content preparation separate from runtime practice data.

## Codex Boundary

Codex advice must not read the mirrored corpus directly.

The only public AI input contract is a typed advice request built from:

- session and variant identity
- exercise metadata
- selected key and target tempo
- aggregate pitch/timing metrics
- weak-step summaries
- optional short user note

Any raw prompt text passed to the local Codex CLI is an internal adapter detail
derived from that request. That boundary keeps the advice lane explicit, local,
and auditable.

## Validation Expectations

Before the app claims a variant is supported, the pipeline should verify:

- every catalog entry resolves to a local source page
- every display asset path exists
- every guided variant resolves to exactly one overlay sidecar
- every overlay region falls within the display asset bounds
- every overlay reference dimension matches the display asset dimensions
- every analysis-enabled variant has enough step metadata to compare captured
  MIDI against expectations

## Related Decisions

Resolved implementation blockers are tracked in [Open Questions](OPEN_QUESTIONS.md).
