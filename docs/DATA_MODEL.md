# Data Model

The data model now covers the full local teaching loop, not only static score
guidance.

The first build needs six data families:

- catalog data for mirrored variants
- overlay data for guidance and analysis expectations
- user state for current preferences and notes
- practice-session data for completed takes
- captured MIDI events
- analysis and advice artifacts

## Common C Conventions

- Public transfer records are plain structs prefixed with `ht_`.
- Mutable services use opaque handles and are not exposed as open structs.
- Public records are C ABI data carriers, not lifetime owners.
- Public typedef names avoid the reserved `_t` suffix.
- Stable identifiers are stored on disk as text keys such as `variant_id` and
  `session_id`.
- Public APIs accept borrowed NUL-terminated identifier strings.
- Modules copy borrowed strings when they need to retain them after a call.
- Captured MIDI timestamps are stored as session-relative integer nanoseconds.
- Fallible load/save operations return `ht_status`.

## Catalog Family

`ht_variant_record` describes one exercise and key variant.

| Field | Purpose |
| --- | --- |
| `exercise_id` | Stable identifier for an exercise family |
| `variant_id` | Stable identifier for one exercise and key combination |
| `exercise_number` | Human-facing Hanon number |
| `title` | Display title |
| `key_name` | Human-facing key label |
| `sort_order` | Stable browser ordering |
| `tempo_text` | Source-derived tempo guidance |
| `repeat_text` | Source-derived repetition guidance |
| `practice_notes` | Source-derived teaching notes |
| `source_page_path` | Local mirrored page path |
| `source_pdf_path` | Local mirrored PDF path when available |
| `display_asset_path` | Canonical runtime image asset |
| `display_asset_width_px` | Width of the canonical display asset |
| `display_asset_height_px` | Height of the canonical display asset |
| `overlay_id` | Referenced overlay when one exists |
| `support_level` | `asset_only`, `guide_only`, `pilot_analysis`, or `analysis_ready` |

`analysis_ready` means the full exercise can be scored. `pilot_analysis` means
only a bounded passage has reviewed overlay and expectation coverage.

## Overlay Family

`ht_overlay_record` holds variant-level overlay metadata.

| Field | Purpose |
| --- | --- |
| `overlay_id` | Stable overlay identifier |
| `variant_id` | Owning variant |
| `step_count` | Number of guided steps |
| `hand_scope` | Left, right, both, or mixed |
| `analysis_enabled` | Whether scored analysis is allowed |
| `coverage_scope` | `bounded_passage` or `full_exercise` |
| `reference_asset_width_px` | Display asset width used when authoring regions |
| `reference_asset_height_px` | Display asset height used when authoring regions |
| `overlay_path` | Sidecar path for the detailed overlay data |

`ht_overlay_step_record` holds step-level guidance and analysis expectations.

| Field | Purpose |
| --- | --- |
| `overlay_id` | Owning overlay |
| `step_index` | Zero-based step order |
| `page_index` | Score page index |
| `cursor_region` | Score rectangle or region id |
| `keyboard_target` | Piano-key grouping for the step |
| `finger_label` | Optional finger hint |
| `step_note` | Optional cue text |
| `expected_pitch_group` | Expected note set for first-pass analysis |
| `timing_window_ms` | Allowed onset window for the step |

## User State Family

`ht_user_state_record` is persisted in SQLite and stays intentionally small.

| Field | Purpose |
| --- | --- |
| `last_variant_id` | Resume the last viewed variant |
| `last_step_index` | Resume the last focused step |
| `target_tempo` | User-owned tempo goal |
| `bookmark_flag` | Lightweight current bookmark marker |
| `user_note` | Short private note for the current variant |
| `last_midi_device_id` | Most recently armed device |
| `updated_at` | Last local edit time |

## Practice Session Family

`ht_session_record` represents one completed or partial take.

| Field | Purpose |
| --- | --- |
| `session_id` | Stable local session identifier |
| `variant_id` | Practiced variant |
| `midi_device_id` | Capturing device id |
| `target_tempo` | Tempo target at take start |
| `started_at` | Wall-clock start timestamp |
| `ended_at` | Wall-clock end timestamp |
| `duration_ms` | Take duration |
| `capture_status` | `capturing`, `completed`, `aborted`, or `invalid` |
| `analysis_status` | `pending`, `complete`, `unsupported`, or `failed` |

## MIDI Event Family

`ht_midi_event_record` stores raw captured events plus normalized timing.

| Field | Purpose |
| --- | --- |
| `session_id` | Owning session |
| `sequence_no` | Stable capture order |
| `event_ns` | Nanoseconds from the start of the capture session |
| `status_byte` | Raw MIDI status byte |
| `data_1` | First MIDI payload byte |
| `data_2` | Second MIDI payload byte |
| `channel` | Decoded MIDI channel when applicable |
| `event_kind` | Decoded note-on, note-off, controller, or other kind |

## Analysis And Advice Family

`ht_analysis_record` stores aggregate local metrics for a completed take.

| Field | Purpose |
| --- | --- |
| `session_id` | Owning session |
| `analyzed_at` | Analysis timestamp |
| `wrong_note_count` | Played notes outside the expected pitch group |
| `missed_note_count` | Expected notes not observed |
| `extra_note_count` | Additional notes outside the guided passage |
| `mean_onset_error_ms` | Average onset deviation |
| `max_late_ms` | Largest late onset |
| `max_early_ms` | Largest early onset |
| `weak_step_count` | Number of flagged steps |
| `summary_text` | Compact local summary for UI and advice input |

`ht_analysis_step_record` stores step-level review output.

| Field | Purpose |
| --- | --- |
| `session_id` | Owning session |
| `step_index` | Analyzed step |
| `pitch_status` | Local pitch result for the step |
| `timing_status` | Local timing result for the step |
| `note_summary` | Compact mismatch summary |

Aggregate analysis and step-level analysis are both persisted. Event ordering is
stable by `(session_id, sequence_no)`; timing comparison uses `event_ns`.

`ht_advice_request_record` is the typed boundary for optional Codex advice.

| Field | Purpose |
| --- | --- |
| `session_id` | Owning session |
| `variant_id` | Practiced variant |
| `exercise_number` | Human-facing Hanon number |
| `exercise_title` | Display title used in advice context |
| `key_name` | Human-facing key label |
| `target_tempo` | Tempo goal at take start |
| `support_level` | Corpus support level used for the analysis |
| `pitch_summary` | Aggregate pitch metrics for advice input |
| `timing_summary` | Aggregate timing metrics for advice input |
| `weak_step_summary` | Compact step or phrase weaknesses |
| `user_note` | Optional short user note |

`ht_advice_record` stores optional Codex-generated teaching output.

| Field | Purpose |
| --- | --- |
| `advice_id` | Stable local advice artifact id |
| `session_id` | Owning session |
| `created_at` | Generation timestamp |
| `prompt_version` | Prompt contract version |
| `codex_command` | Executed local command line |
| `request_summary` | Compact prompt payload stored for auditability |
| `advice_markdown` | Generated advice text |
| `generation_status` | `complete`, `timeout`, `failed`, or `stubbed` |

## Stable Identifier Rules

- `exercise_id` stays stable even if titles change.
- `variant_id` stays stable across asset refreshes for the same exercise and
  key pairing.
- `overlay_id` must not encode ephemeral filesystem details.
- `session_id` and `advice_id` are local runtime identifiers and never replace
  source-derived ids.

## Explicitly Out Of Model

The first build does not need first-class models for:

- audio waveform capture
- real-time coaching state
- teacher accounts
- remote sync
- expressive phrasing grades
- fingering correctness grades

## Review Notes

The key question is no longer whether the model is small. It is whether the
model keeps source-derived corpus data, app-authored overlays, captured
performance evidence, and generated advice cleanly separated.
