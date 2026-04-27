# Review - Hanon 01C Pilot Analysis Data Slice

Date: 2026-04-27

## Scope Reviewed

Target repository: `workspace/product/hanon-trainer`

This review covers the proposed implementation slice to promote
`hanon-01-c` from `asset_only` to `pilot_analysis`, add first-measure overlay
data, and update deterministic post-session analysis so one overlay step can
score a pitch group instead of one `NOTE_ON`.

Current local status matches the branch hygiene assumption: the working tree is
on `development...origin/development` with only these known untracked files:

- `docs/PORTABLE-C-TEXT-PARSING.md`
- `tests/fixtures/pc4_synthetic_capture.tsv`

## Verdict

The plan is directionally sound and matches the existing architecture: catalog
and overlay metadata stay TSV-backed, fixture evidence enters through SQLite,
and `performance_analysis` remains the single deterministic scoring boundary.

Before implementation, tighten the plan in three places:

- define exact midpoint ownership boundaries for step candidate grouping
- decide whether `coverage_scope=first_measure` is a new contract token or
  whether the existing `bounded_passage` token should be used
- strengthen production overlay and fixture tests to check all eight steps, not
  only representative rows

With those improvements, the slice is implementable without public API changes.

## Blocking Clarification

### Coverage Scope Token

The requested production overlay row uses:

```text
coverage_scope=first_measure
```

Current docs define `coverage_scope` as `bounded_passage` or `full_exercise` in
`docs/DATA_MODEL.md`, and earlier sprint docs also use bounded-passage wording.
The code currently accepts arbitrary text, so this will not fail at runtime, but
it does create contract drift.

Proposed resolution:

- If `first_measure` is intentional, update the data-model and content-pipeline
  docs in the same implementation commit as the production overlay row.
- If no new vocabulary is needed, keep `coverage_scope=bounded_passage` and
  express first-measure specificity in `step_note` values or a future passage
  field.

Because the user story explicitly names `coverage_scope=first_measure`, the
implementation should either update the contract docs or record the deliberate
deviation in the commit message and tests.

## Proposed Analysis Contract

### Candidate Ownership

Use event timestamps, not consumption order, to assign `NOTE_ON` events to
steps. The database loads events ordered by `sequence_no`, but the ownership
rule is time-based and should be written that way internally.

Recommended exact rule:

- `expected_ns(step_index) = step_index * 1000000000`
- each step owns a half-open interval: `[lower_ns, upper_ns)`
- for step 0, `lower_ns` is unbounded or effectively before any valid capture
  timestamp
- for step `i > 0`, `lower_ns = midpoint(expected_ns(i - 1), expected_ns(i))`
- for step `i < step_count - 1`, `upper_ns = midpoint(expected_ns(i), expected_ns(i + 1))`
- for the final step, `upper_ns = expected_ns(last_step) + 500000000`
- an event exactly on an upper midpoint belongs to the next step
- `NOTE_ON` events outside every step interval count as extra notes outside the
  guided passage

This preserves normal early/late detection while preventing tail events after
the pilot passage from being silently absorbed by the last step.

### Pitch Group Matching

Keep the public API unchanged and implement pitch-group scoring inside
`performance_analysis.c`.

The one-to-one matching rule should explicitly handle duplicate pitches, even
though the Hanon pilot groups are octave pairs and do not contain duplicates.
Use a small local `matched_expected[HT_MAX_EXPECTED_PITCHES]` array or
equivalent expected-slot matching rather than a simple `pitch_matches` boolean.

Aggregate scoring should follow the requested formulas:

- `matched_count`: candidates matched one-to-one to expected pitches
- `wrong_note_count += min(candidate_count, expected_count) - matched_count`
- `missed_note_count += max(0, expected_count - candidate_count)`
- `extra_note_count += max(0, candidate_count - expected_count)`

Status mapping should remain step-level:

- `HT_PITCH_MATCH`: all expected pitches matched and no extra candidate exists
- `HT_PITCH_MISSED`: fewer candidate notes than expected, even if one candidate
  is also the wrong pitch
- `HT_PITCH_WRONG`: enough notes exist, but at least one pitch is wrong or extra

For a two-note step with no candidates, aggregate `missed_note_count` should
increase by `2`, not `1`. The requested missing-one-hand fixture should still
expect `missed_note_count=1`.

### Timing Metrics

For timing, use the earliest candidate `NOTE_ON` in the group:

- no candidates: `HT_TIMING_MISSING`
- otherwise compare earliest candidate onset against `step_index * 1000ms`
- `mean_onset_error_ms` should continue to average across timed steps, not
  across individual notes
- a weak step should be counted once if either pitch or timing is weak

The `late-onset.tsv` fixture must delay both notes in the selected pitch group
by `120ms`. If only one hand is delayed and the other note remains on time, the
earliest-onset rule will intentionally keep that group on time.

## Production Overlay Test Improvements

The requested `tests/test_overlay_store.c` update should check the entire
production overlay, not only steps 0 and 4.

Add assertions for:

- `hanon-01-c-pilot-001` exists in the production overlay store
- `variant_id == "hanon-01-c"`
- `step_count == 8`
- `hand_scope == "both"`
- `analysis_enabled == true`
- `coverage_scope` matches the resolved contract token
- reference dimensions are exactly `2480 x 3508`
- steps 0 through 7 all exist
- every step has `page_index == 0`
- every step has `timing_window_ms == 80`
- every step has `expected_pitch_count == 2`
- expected pitch groups exactly match:
  - step 0: `48 60`
  - step 1: `50 62`
  - step 2: `52 64`
  - step 3: `53 65`
  - step 4: `55 67`
  - step 5: `53 65`
  - step 6: `52 64`
  - step 7: `50 62`
- every rectangle is non-empty and inside `0 <= x`, `0 <= y`,
  `x + width <= 2480`, `y + height <= 3508`

This makes the real corpus row the regression target and avoids a future bad
row passing because only representative steps were checked.

## Fixture Test Improvements

The fixture plan is good. Add a dedicated CTest-backed C test, for example
`tests/test_hanon_01c_pilot_analysis.c`, rather than overloading the existing
synthetic test too much.

Recommended test structure:

- open production `corpus`
- insert one SQLite session per fixture with `variant_id="hanon-01-c"`
- parse `tests/fixtures/hanon-01-c/*.tsv`
- insert every fixture row through `ht_db_append_midi_event`
- run `ht_analysis_run_session`
- load aggregate results with `ht_db_load_analysis`
- load step results with `ht_db_load_analysis_steps`
- assert exactly 8 stored step results for every fixture

Fixture expectations:

- `perfect.tsv`: aggregate pitch and timing counts are zero; all 8 steps are
  `HT_PITCH_MATCH` and `HT_TIMING_ON_TIME`
- `wrong-note.tsv`: exactly one substituted pitch; `wrong_note_count=1`,
  `missed_note_count=0`, and the selected step is `HT_PITCH_WRONG`
- `missing-note.tsv`: remove one hand from one group; `missed_note_count=1`,
  the selected step is `HT_PITCH_MISSED`, and timing remains on time because
  one candidate note exists
- `late-onset.tsv`: delay both notes in one group by `120ms`; expect
  `max_late_ms=120`, one weak step, pitch match, and timing late on that step

Use numeric `event_kind=1` for `HT_MIDI_EVENT_NOTE_ON`, matching the existing
SQLite fixture convention. Keep `sequence_no` monotonic and aligned with
`event_ns` so the fixture remains easy to audit.

Add one small synthetic regression for midpoint ownership:

- a note at `490ms` belongs to step 0 and is late
- a note at `510ms` belongs to step 1 and is early

That protects the core change from accidentally falling back to the old
single-event consumption model.

## Implementation Notes

### Avoid Public API Churn

The requested behavior can stay internal to `performance_analysis.c`. A compact
private helper set is enough:

- collect candidate `NOTE_ON` indices for a step interval
- score pitch candidates against one overlay step
- derive timing status from the earliest candidate
- append aggregate counters into `ht_analysis_record`

No public structs need new fields for this slice.

### Preserve Single-Note Behavior

The existing synthetic fixture corpus uses one expected pitch per step. Keep
`tests/test_performance_analysis.c` passing unchanged where possible. If helper
signatures change, update only the test mechanics, not the expected behavior.

The current `wrong-late-same-step-session` case is especially valuable because
it proves a late wrong note at `120ms` still belongs to step 0, not step 1.

### Fixture Parser

If the new C test parses fixture TSVs directly, reuse the same standard-C cursor
style described in `docs/PORTABLE-C-TEXT-PARSING.md`. Do not use `strsep` or
`strtok`; empty fields are meaningful in this project family.

### Coordinate Entry

Record the coordinate convention in the overlay review notes or the commit
message:

- origin is the top-left pixel of `corpus/runtime/assets/hanon-exercise-01-c.ppm`
- `x` and `y` are top-left
- `width` and `height` are positive dimensions
- right and bottom edges are exclusive for bounds checks

This matters because the plan requires rectangles measured directly from the
PPM and kept inside `2480 x 3508`.

## Documentation Updates To Include

The implementation should update documentation that currently describes the
production corpus as asset-only or header-only:

- `README.md`: feature list currently says one real `hanon-01-c` asset-only row
  and header-only production overlays
- `corpus/README.md`: tracked corpus summary currently says the same
- `docs/manual-input/hanon-01-c-checklist.md`: status and guardrails currently
  say not to promote above `HT_SUPPORT_ASSET_ONLY`
- companion docs for changed tests or sources if their stated test strategy is
  no longer accurate

If `coverage_scope=first_measure` is retained, also update:

- `docs/DATA_MODEL.md`
- `docs/CONTENT_PIPELINE.md`
- any sprint or review docs that define coverage scope vocabulary

## Commit Plan Improvements

Use two commits on `feature/hanon-01c-pilot-analysis-data`:

1. Add only `docs/PORTABLE-C-TEXT-PARSING.md`.
2. Add Hanon pilot corpus rows, analysis internals, fixtures, tests, and any
   documentation updates required by the new production status.

Before the first commit, fetch or otherwise confirm that local
`origin/development` is current. `development...origin/development` being clean
only proves the local tracking ref matches the local branch.

Use explicit staging:

```sh
git add docs/PORTABLE-C-TEXT-PARSING.md
```

Do not use broad staging that could include
`tests/fixtures/pc4_synthetic_capture.tsv`.

## Required Verification

The proposed verification set is appropriate:

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
cmake -S . -B build-fake -G Ninja -DHT_MIDI_BACKEND=FAKE
cmake --build build-fake
ctest --test-dir build-fake --output-on-failure
git diff --check
```

Add one pre-commit sanity check:

```sh
git status --short
```

Expected result before committing the implementation slice: no tracked
surprises, and `tests/fixtures/pc4_synthetic_capture.tsv` still untracked.

## Final Recommended Change To The Plan

Add a short "Resolved Contracts" subsection before implementation starts:

- final `coverage_scope` token
- exact half-open midpoint interval rule
- final-step upper boundary rule
- group-level timing denominator
- fixture `event_kind` encoding
- production overlay rectangle coordinate convention

That small addition will remove the main ambiguity while preserving the plan's
current scope.
