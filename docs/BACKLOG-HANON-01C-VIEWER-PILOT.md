# Backlog - Hanon 01C Viewer Pilot

Branch: `feature/hanon-01c-viewer-pilot-backlog`

Date: 2026-04-27

Reviewed: 2026-04-27

## Review Outcome

The slice is valid and should stay score-first: headless render core plus
`ht_viewer_snapshot`, no interactive SDL window, no live MIDI capture, and no AI
surface.

The original backlog had the right user proof, but it needed a tighter contract
in four places before implementation:

- settle the public/private API decision up front
- define asset path resolution so `--corpus corpus` does not accidentally become
  `corpus/corpus/runtime/assets/...`
- separate request failures from degraded content states
- make layout, projection, and pixel tests deterministic enough for CTest

## Decision

Build a headless-renderable viewer pilot for the already-merged `hanon-01-c`
bounded analysis passage.

This is the smallest user-visible step after the MIDI HAL and analysis work:
the code already knows the variant, score asset path, overlay rectangles,
expected pitch groups, and analysis results. The missing proof is that the app
can present that data as a practice surface instead of only as tests and TSVs.

## Slice Goal

Render the Hanon 01C score asset with one active overlay rectangle and step
navigation across the eight pilot steps, without requiring real MIDI hardware,
real capture, real Codex advice, or a desktop window in CI.

The slice should prove:

- production catalog lookup resolves `hanon-01-c`
- production overlay lookup resolves `hanon-01-c-pilot-001`
- the score asset can be loaded when present
- the score can be fit into a viewport while preserving aspect ratio
- overlay coordinates can be projected from asset space into viewport space
- the active step can be highlighted visibly
- missing asset, missing overlay, and invalid step states are explicit
- the implementation remains usable in CI without opening a desktop window

## Implementation Contract

Extend the existing `score_renderer` boundary instead of adding a parallel
viewer subsystem.

Public API guidance:

- add the smallest needed public records/functions to
  `include/hanon_trainer/score_renderer.h`
- keep public identifiers under the project-owned `ht_` prefix
- keep public records caller-owned, fixed-size, and C ABI-style
- keep mutable renderer state behind the existing opaque `ht_score_renderer`
  handle
- do not expose `SDL_Surface`, `SDL_Rect`, or other SDL types in public headers
- keep PPM parsing, surface creation, and pixel writing private to
  `src/score_renderer.c` unless a later slice needs a separate internal module
- add companion docs for every new or changed C source, header, tool, and test
  file

Recommended public shape:

- `ht_score_renderer_view_request`
  - `corpus_root`
  - `asset_root`
  - `variant_id`
  - `step_index`
  - `viewport_width_px`
  - `viewport_height_px`
  - optional `output_path`
- `ht_score_renderer_view_result`
  - `variant_id`
  - `overlay_id`
  - resolved asset path
  - source asset dimensions
  - viewport dimensions
  - fitted score rectangle
  - projected active overlay rectangle
  - catalog, overlay, asset, and render status fields
- `ht_score_renderer_render_view(...)`
  - fills the result with diagnostics whenever the request is valid enough to
    inspect
  - returns non-`HT_OK` for invalid arguments, corrupt data, unsupported PPM, or
    failed output writes
  - reports missing content in the result status fields so the CLI can emit a
    precise message and exit code

This keeps the API useful for tests and the later interactive shell without
committing to a full UI model.

## Path Policy

`--corpus` points to the directory that contains `catalog/` and `overlays/`.
That is the value passed to `ht_catalog_open` and `ht_overlay_store_open`.

Catalog asset paths are currently stored as project-root-relative paths, for
example:

```text
corpus/runtime/assets/hanon-exercise-01-c.ppm
```

The viewer request therefore carries both:

- `corpus_root`: TSV root, usually `corpus`
- `asset_root`: path base for catalog asset paths, usually `.`

Resolution order:

1. If `display_asset_path` is absolute, use it as-is.
2. Otherwise join `asset_root` with `display_asset_path`.
3. Synthetic test corpora may set `asset_root` to the fixture directory and use
   fixture-relative asset paths.

Do not join `corpus_root` directly with the production `display_asset_path`
unless the catalog row is changed in the same patch. That would produce the
wrong path for the current corpus.

## Rendering Shape

Use SDL software surfaces only.

Required constraints:

- no `SDL_CreateWindow` in required tests
- no OpenGL or GPU dependency
- no image library dependency
- no text rendering dependency for the pilot snapshot path
- no runtime PDF rendering dependency
- PPM loading stays local and deterministic
- pixel checks verify that the score background and active overlay were
  actually rendered

The current `SDL2` and `SDL_ttf` link probe can remain, but the viewer render
path should not depend on `SDL_ttf`.

## Non-Goals

- live MIDI capture
- app session orchestration
- review panel
- AI advice UI
- browser rail
- polished layout
- text rendering beyond optional CLI diagnostics
- editing overlay coordinates in-app
- full Hanon exercise coverage
- strict `analysis_ready`
- interactive SDL window

## Epic 1: Viewer Data Contract

Add a small viewer-facing model that composes existing records instead of
inventing a parallel schema.

Tasks:

- extend `include/hanon_trainer/score_renderer.h`
- update `include/hanon_trainer/score_renderer.h.md`
- define request/result records with fixed-size fields
- include `corpus_root` and `asset_root` separately
- include optional output path as an empty string when no snapshot should be
  written
- include per-stage status fields in the result
- define the step index as zero-based
- reject zero viewport dimensions with `HT_ERR_INVALID_ARG`
- do not expose SDL handles or SDL types from public headers

Acceptance:

- a caller can ask for `hanon-01-c`, step `0`, viewport `1280 x 900`
- the result reports source dimensions `2480 x 3508` when the asset exists
- the fitted score rectangle preserves aspect ratio
- the projected overlay rectangle is inside the fitted score rectangle
- invalid step indexes are distinguishable from missing asset failures
- public headers still pass the existing header-independence and identifier
  discipline checks

## Epic 2: PPM Asset Loading

Implement a minimal PPM loader for the runtime score asset format already used
by the corpus.

Tasks:

- load binary P6 PPM files
- parse width, height, and max value
- support PPM header whitespace and comment lines
- reject unsupported magic values and malformed headers
- reject non-255 max values for this slice
- reject dimensions that overflow buffer-size calculations
- reject files whose raster payload is shorter than expected
- ignore trailing bytes only if the decision is documented in the test
- store pixels in renderer-owned memory or an SDL-owned software surface
- add tiny synthetic PPM fixtures for tests
- keep Hanon source-derived PPM files out of git unless a later content
  decision changes that

Acceptance:

- synthetic PPM fixture loads in CI
- malformed PPM fixtures fail deterministically
- short-payload and bad-max-value fixtures fail deterministically
- production asset load is covered by an optional smoke path when the asset
  exists locally

## Epic 3: Layout And Overlay Projection

Convert overlay rectangles from score-asset coordinates to viewport
coordinates.

Tasks:

- compute fitted score rectangle for a viewport
- center the fitted score image within the viewport
- preserve aspect ratio with deterministic integer math
- use 64-bit intermediates for multiplication
- project rectangle edges from asset coordinates into viewport coordinates
- ensure projected rectangles remain at least one pixel wide and high when the
  source rectangle is non-empty
- reject source rectangles outside asset bounds
- reject overlays whose reference dimensions do not match the catalog asset
  dimensions
- cover all eight `hanon-01-c-pilot-001` steps

Recommended fit rule:

- choose the largest rectangle that fits entirely inside the viewport
- floor scaled width/height rather than rounding outward
- center with integer division of the remaining space

Acceptance:

- every production pilot step projects into the fitted score area
- step `0` and step `7` project to different x positions in left-to-right order
- viewport changes produce stable, deterministic projected rectangles
- an overlay authored against dimensions other than `2480 x 3508` is rejected
  for the current production variant

## Epic 4: Headless Score Rendering

Render into an SDL software surface and optionally write a PPM snapshot.

Tasks:

- create only the SDL software surfaces needed for rendering
- render the score pixels into the fitted score rectangle
- clear the viewport to a deterministic background color
- draw the active overlay rectangle with a high-contrast outline
- keep overlay fill out of this slice unless the test requires it
- write a P6 PPM snapshot for tests and local inspection
- return explicit status for missing score assets
- keep the render path independent of a display server

Acceptance:

- CI can render a synthetic score plus overlay without a window
- pixel checks prove the output is nonblank
- pixel checks prove the overlay outline exists at the projected rectangle
- rendering `hanon-01-c` with the real asset works locally when the PPM exists
- missing production PPM does not create an empty or misleading snapshot

## Epic 5: Minimal Viewer CLI

Add a small executable for local smoke testing.

Suggested command:

```sh
./build/ht_viewer_snapshot \
  --corpus corpus \
  --asset-root . \
  --variant hanon-01-c \
  --step 0 \
  --viewport 1280x900 \
  --out build/hanon-01c-step0.ppm
```

Tasks:

- add `tools/ht_viewer_snapshot.c`
- add `tools/ht_viewer_snapshot.c.md`
- parse only the required flags
- load catalog and overlay stores from `--corpus`
- resolve catalog asset paths through `--asset-root`
- resolve the variant and active step
- call the headless renderer
- print a compact status line with:
  - variant id
  - overlay id
  - step index
  - asset dimensions
  - fitted score rectangle
  - projected overlay rectangle
  - output path
- return distinct non-zero codes for invalid CLI input, missing asset, missing
  overlay/step, corrupt asset, and output write failure

Acceptance:

- command succeeds with a synthetic fixture corpus in CI
- command succeeds for `hanon-01-c` when the local score asset exists
- command fails cleanly when the production asset is absent
- command does not open a window

## Epic 6: Tests And CI

Tests should prove rendering behavior without requiring the real third-party
score asset in git.

Tasks:

- add `tests/test_score_renderer_layout.c`
- add `tests/test_score_renderer_layout.c.md`
- add `tests/test_score_renderer_ppm.c`
- add `tests/test_score_renderer_ppm.c.md`
- add `tests/test_hanon_01c_viewer_projection.c`
- add `tests/test_hanon_01c_viewer_projection.c.md`
- add `tests/tools/check_viewer_snapshot_cli.py`
- add a synthetic fixture corpus with a tiny PPM asset
- add malformed PPM fixtures for parser failures
- keep the existing asset-generation test as the only place that touches the
  real Hanon PDF-to-PPM pipeline
- keep production-asset tests optional unless the asset was generated in that
  build

Acceptance:

- default backend CTest passes on Linux
- fake backend CTest passes on Linux
- macOS and Windows CI do not need a GUI display
- missing production PPM does not fail CI unless the test explicitly generated
  it in that build
- `git diff --check` is clean

## Epic 7: Documentation Updates

Update docs only after implementation behavior exists.

Tasks:

- update `README.md` status from compile/link SDL boundary to headless viewer
  render proof
- update `docs/UI_CONCEPT.md` with the viewer pilot limitation
- update `docs/CONTENT_PIPELINE.md` with snapshot-render smoke expectations
- update source companion docs for renderer changes

Acceptance:

- docs say exactly what the viewer can and cannot do
- no doc claims interactive capture, live scoring, AI guidance, or full UI
  exists

## Proposed Implementation Order

1. Add request/result records and CMake targets.
2. Add deterministic layout/projection helpers behind `score_renderer`.
3. Add PPM loader with synthetic and malformed fixture tests.
4. Add headless render-to-PPM path and pixel checks.
5. Add production Hanon projection tests.
6. Add `ht_viewer_snapshot` CLI and Python CLI smoke test.
7. Update README, UI/content docs, and companion docs.
8. Run default and fake-backend CTest.

## Resolved Review Questions

Expose a public viewer API?

Yes, but only as the smallest extension of `score_renderer.h`. The later
interactive shell can reuse it, and tests can exercise the same contract as the
CLI.

How should missing production score assets behave?

The renderer result should report missing asset status explicitly, and the CLI
should return a distinct non-zero exit code. Do not generate a placeholder
snapshot that could be mistaken for a successful render.

Should overlay drawing use outline only or outline plus translucent fill?

Use outline only for this pilot. It is simpler to pixel-test and avoids hiding
notation before a real UI review exists.

Should the interactive window be allowed in this branch?

No. Keep the interactive SDL window as the next slice after the headless
renderer and snapshot CLI pass on CI.

## Definition Of Done

The slice is complete when:

- `ht_viewer_snapshot` renders a synthetic score fixture in CI
- `ht_viewer_snapshot` renders `hanon-01-c` locally when the generated PPM
  exists
- all eight production pilot overlay steps project correctly
- missing asset and missing overlay paths fail with clear status
- no required test opens a desktop window
- companion docs are present for new C files
- default and fake-backend CTest pass
- `git diff --check` is clean
