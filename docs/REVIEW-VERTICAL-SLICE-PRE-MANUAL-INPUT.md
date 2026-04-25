# Review - Vertical Slice Plan To Manual Input

This review evaluates the proposed first implementation slice on `development`
that stops exactly before manual musical input.

The plan was reviewed against the existing pack:
[Sprint 001](SPRINT-001-PILOT-TRAINER.md),
[Sprint 001 review](REVIEW-SPRINT-001.md),
[Backlog refinement](BACKLOG-REFINEMENT.md),
[Local spec](LOCAL_SPEC.md),
[Architecture](ARCHITECTURE.md),
[Data model](DATA_MODEL.md),
[Content pipeline](CONTENT_PIPELINE.md),
[Open questions](OPEN_QUESTIONS.md),
[ADR-0009](ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md),
the `.gitignore`, and the existing `scripts/setup-dev-env.sh`.

## Verdict

The plan is **conditionally acceptable**.

The technical shape is right: it stops before any musical claim, it preserves
`pilot_analysis` versus `analysis_ready` discipline, it reuses the established
`ht_status`, opaque-handle, and acyclic-graph contracts, and it pushes both
SDL2 and ALSA down to compile/link boundaries that match the existing dev-env
probes.

What blocks unconditional acceptance is a small set of contract gaps and a
companion-doc rule that, if landed as written, will create more drift than it
prevents. The blockers below are surgical, not structural.

## What The Plan Gets Right

- The slice **stops before manual musical input**, and the manual-input packet
  is treated as data, not as code. This matches Sprint 001's review-gate model.
- Synthetic test data uses **clearly fake IDs** (e.g. `test-variant`) and does
  not reuse `hanon-01-c`. This protects the `pilot_analysis` promotion gate
  documented in `BACKLOG-REFINEMENT.md`.
- The compile-time graph remains acyclic: `midi_capture` is stubbed and does
  not depend on `sqlite_store`; orchestration drains events into the DB.
- AI advice is reachable through `advice_orchestrator` but resolves to
  `HT_GENERATION_STUBBED`, matching the [Sprint 001 review's AI Stub
  Placement](REVIEW-SPRINT-001.md#ai-stub-placement) decision.
- Generated PPM and SQLite DBs remain ignored, matching the existing
  `.gitignore` policy and `corpus/runtime/assets/.gitkeep` convention.
- The dev-env script is kept as the canonical installer/verifier rather than
  being relitigated.
- C11 minimum, `-Wall -Wextra -Wpedantic`, public-header independence, opaque
  handles, `ht_` prefix, and no `_t` public typedef suffix are all consistent
  with [LOCAL_SPEC](LOCAL_SPEC.md) and [ADR-0009](ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md).

## Blocking Issues

These should be resolved before commit 1 lands.

### B1. Replace external book references with project contracts

The plan cites `Modern C Library` chapter and appendix numbers as the source
of truth for opaque handles, status returns, identifier rules, and C11/C23
posture. The repository already owns those rules in
[LOCAL_SPEC](LOCAL_SPEC.md) and
[ADR-0009](ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md).
External chapter numbers cannot be pinned to an edition and will not survive a
public-OSS review. Restate the rules as references to `LOCAL_SPEC.md` and
`ADR-0009`. Keep any external book in a single `Further reading` line at most.

### B2. Tighten the `_t` and leading-underscore checks

A naive grep for `_t` or leading underscores will flag standard headers,
compiler attributes (`__attribute__`, `__has_include`), C keywords
(`_Static_assert`, `_Atomic`), and standard typedefs (`size_t`, `uint8_t`,
`int64_t`) that the spec actually requires. Specify the checks as:

- forbid only `typedef ... ht_<name>_t` and any project-defined `typedef ...
  _<name>` in files under `include/hanon_trainer/` and `src/`.
- forbid only project-defined identifiers that begin with an underscore; allow
  standard and compiler-reserved tokens.

Otherwise the test will either be permanently broken or produce false-clean
results.

### B3. Disambiguate the "synthetic pilot catalog entry"

The plan says the catalog must "resolve the synthetic pilot catalog entry and
generated asset path" while also using "fake test IDs only". Those two
sentences disagree. Pick one of:

- **A.** Ship one real catalog row for `hanon-01-c` with
  `support_level = HT_SUPPORT_ASSET_ONLY`, no overlay, and use it for the
  asset-readiness path. Use fake IDs only inside the parser unit tests
  (in-memory or `tests/fixtures/`).
- **B.** Ship no catalog rows; the catalog test fixtures live entirely under
  `tests/fixtures/` with synthetic IDs and the asset-readiness test resolves a
  fake variant against a fake asset.

Option A matches `CONTENT_PIPELINE.md` step 2, makes the PPM generation test
meaningful, and keeps `corpus/` honest. Recommend A and state it.

### B4. PPM asset generation must not fail on a fresh clone

`corpus/source-pdfs/*.pdf` and `corpus/runtime/assets/*` are both ignored.
A CTest target that generates `hanon-exercise-01-c.ppm` will fail on any
checkout where the developer has not yet mirrored the source PDF. State the
behavior explicitly:

- if `corpus/source-pdfs/hanon-exercise-01-c.pdf` is absent, the
  asset-generation test reports `SKIPPED`, not failure.
- generation runs as a CMake `add_custom_command` whose output the verify-test
  consumes; verify-test depends on that custom target so parallel builds do
  not race.
- the verify-test still runs against a precomputed PPM if one exists, even
  without `pdftoppm`, so CI machines without poppler can still validate the
  asset contract.

### B5. State the test framework choice

CTest with one `int main` per test executable using `<assert.h>` is the
project-fitting choice and matches the no-half-abstractions tone of the
repo. Pin it. Without that, ticket reviewers will bikeshed Unity, cmocka,
greatest, and µnit.

### B6. Define `HT_OK` versus `HT_ERR_UNSUPPORTED` for the advice stub

The Sprint 001 review uses the phrase "stubbed or unsupported result". Those
are different `ht_status` returns. The advice orchestrator stub should:

- return **`HT_OK`** with `out_advice->generation_status =
  HT_GENERATION_STUBBED` and a populated audit record.

`HT_ERR_UNSUPPORTED` would imply the call was invalid, which contradicts the
"local analysis must remain visible" rule. State this explicitly so the test
asserts the right pair.

### B7. State headless SDL behavior

A `score_renderer` link/compile probe must not call `SDL_Init(SDL_INIT_VIDEO)`
or any function that requires a display. State that the probe links the
project's renderer translation unit against `SDL2` and `SDL2_ttf` and exercises
only no-op or version-query entry points. Otherwise CI without `$DISPLAY`
fails the slice.

## Significant Concerns

These are not blocking but should be addressed before commit 5.

### S1. The companion-doc rule will outweigh its value as written

Requiring `src/foo.c.md`, `include/hanon_trainer/foo.h.md`, and
`tests/foo_test.c.md`, each with seven sections (Purpose, Theoretical Model,
Architecture Role, Implementation Contract, Modern C Notes, Ownership And
Failure Modes, Test Strategy), creates a documentation surface roughly three
times the size of the code itself.

Cost:

- the existing `LOCAL_SPEC.md` already documents ownership, status, capacity,
  and precondition contracts at the prototype level; companion docs duplicate
  it.
- the CTest "every `.c` and `.h` has a sibling `.md`" check is shallow:
  presence is not accuracy. Drift will start at the first refactor.
- the rule taxes test files most, where the code itself is the
  documentation.

Recommended scope cut:

- keep companion docs **only for public headers** under
  `include/hanon_trainer/*.h`; they are the public surface and benefit from a
  long-form companion that complements the in-header Doxygen.
- drop the requirement on `.c` and `_test.c`. If a `.c` needs design notes,
  they belong in the matching public header companion or in `ARCHITECTURE.md`.
- if companion docs on `.c` are kept, collapse the seven sections to two:
  *Purpose* and *Why this file exists*. The other five duplicate spec docs.

### S2. Add an acyclic-graph test

[`ARCHITECTURE.md`](ARCHITECTURE.md) and
[ADR-0009](ADR/ADR-0009-c-library-boundaries-and-interface-discipline.md)
require an acyclic compile-time dependency graph. The plan does not test it.
Add a small CTest that, for each public header, scans its `#include
"hanon_trainer/..."` lines and asserts they form a DAG matching the documented
edges. Cheap, durable, and prevents a class of regressions before they take
hold.

### S3. State the C23 fallback header location

The plan reasserts "C23 only behind fallbacks". Create
`include/hanon_trainer/internal/c23_fallback.h` (or similar) as an empty
placeholder during commit 2. Without a place to land, the rule will be
honored breach-by-breach.

### S4. Specify SQLite test DB lifecycle

Tests should use either `:memory:` databases or per-test temporary files
under the build tree. State this in the plan so reviewers do not have to
discover it ticket-by-ticket. WAL journal posture should be noted at the
schema-migration level for the file-backed case.

### S5. Drop the redundant ALSA compile probe in CTest

`scripts/setup-dev-env.sh` already runs an ALSA compile probe against the
system. The CTest equivalent should test the **project's** `midi_capture`
stub linking against ALSA, which proves the build-system link line. A bare
`<alsa/asoundlib.h>` smoke probe inside CTest duplicates the dev-env script.

### S6. Define manual-input packet posture in git

State explicitly:

- `docs/manual-input/hanon-01-c-checklist.md` is committed.
- `docs/manual-input/hanon-01-c-overlay.template.tsv` is committed with only
  the header row.
- `docs/manual-input/hanon-01-c-fixture.template.tsv` is committed with only
  the header row.
- the templates use the **same TSV schema** as production overlay sidecars
  (so manual input is data, not a different file format).
- no row of either template may carry a real Hanon claim until reviewer
  signoff.

### S7. Synthetic fixtures must use the `2480 x 3508` reference basis

`BACKLOG-REFINEMENT.md` requires every overlay region to fall inside the
canonical asset bounds. Synthetic test fixtures should reuse the same
reference dimensions even though the asset itself is fake, so the loader
contract under test is exactly the contract real data will hit.

## Minor Items

- **Commit 5 is overloaded.** Split into 5a (companion docs and existence
  test) and 5b (manual-input packet skeleton). They review independently.
- **`corpus/runtime/assets/.gitkeep` is required by the .gitignore allowlist.**
  Confirm it is committed at or before commit 3.
- **Cross-link the slice plan to ADRs in commit 2.** The CMake scaffold
  commit message should reference ADR-0001 (SDL2), ADR-0005 (SQLite), and
  ADR-0009 (C boundaries). It is cheap and survives well.
- **Public-header independence test wording.** Specify the test as: for each
  `include/hanon_trainer/*.h`, compile a 3-line `.c` containing only
  `#include "hanon_trainer/<header>.h"` and `int main(void){return 0;}`
  with `-std=c11 -Wall -Wextra -Wpedantic`. Otherwise reviewers will guess.
- **CMake fatal-error on non-Linux.** Match the dev-env script's posture; the
  scaffold should refuse to configure on non-Unix or print a clear warning.
- **Stub completeness.** State that every public prototype in
  `midi_capture.h` and `codex_cli_adapter.h` has a defined implementation
  that returns a documented status; no missing symbols.
- **DCO and GPG posture.** Either set `commit.gpgsign` and the trailer in
  repo config, or note the developer is responsible locally. Do not leave it
  implicit.

## Recommended Scope Edits (Deletion Pressure)

Apply this pressure to keep the slice from gaining weight before manual input:

1. **Cut companion docs on `.c` and `_test.c`.** Keep only `*.h.md` companions
   for public headers. (See S1.)
2. **Cut the bare ALSA probe at CTest.** Keep one project-level link probe;
   the dev-env script covers the system probe. (See S5.)
3. **Demote PPM generation to an opt-in CTest skip path** rather than a hard
   acceptance gate. (See B4.)

If a reader cannot defend an item against these cuts, drop it.

## Recommended Commit Plan Adjustments

1. Add `scripts/setup-dev-env.sh` and `corpus/runtime/assets/.gitkeep`.
2. Add CMake scaffold, public C API skeleton, `c23_fallback.h` placeholder,
   non-Linux guard, public-header independence test, identifier-discipline
   tests with the precise wording from B2.
3. Add catalog and overlay readiness path; ship one real `hanon-01-c` catalog
   row with `support_level = HT_SUPPORT_ASSET_ONLY`; PPM generation test with
   skip-on-missing-source semantics; acyclic-graph test (S2).
4. Add SQLite store, schema migration, fixture insertion, `:memory:` test
   posture, and synthetic analysis cases (perfect, wrong-note, missing-note,
   late-onset). State `HT_GENERATION_STUBBED` advice path here.
5. (a) Add public-header companion docs only.
   (b) Add manual-input packet skeleton with TSV templates that share the
   production schema.

Do not push automatically.

## Acceptance Gates Edits

Add these gates to the slice's acceptance bar:

- identifier-discipline checks are precise (B2) and do not flag standard
  headers.
- `hanon-01-c` resolves through the catalog with
  `HT_SUPPORT_ASSET_ONLY` and matches the `2480 x 3508` reference.
- PPM verify-test reports `SKIPPED` rather than failing on a fresh clone.
- the test framework choice is stated and uniform.
- `ht_advice_generate_for_session` returns `HT_OK` with
  `HT_GENERATION_STUBBED` for the stubbed adapter.
- the `score_renderer` probe never opens a window or initializes SDL video.
- the acyclic-graph test exists and passes.
- the public-header companion-doc set is the only companion-doc gate.
- the manual-input packet exists with header-only TSV templates and a
  signoff-required checklist.

## Closing

The slice plan reaches the right stopping point and respects the established
boundaries. Address the seven blockers above before commit 1, and apply the
deletion pressure to the companion-doc rule and the redundant ALSA probe so
the slice does not gain weight on its way to manual input.

After those edits, the plan is implementable as documented and lands the
project at the cleanest possible point to drop in real Hanon overlay data.
