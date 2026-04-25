# UI Concept

The app remains score-first.

The user should feel like they are practicing from a score, capturing a take,
and then reviewing specific feedback. The UI must not turn into a chat client
or a generic MIDI utility.

## Primary Layout

The main window uses four persistent areas plus one on-demand review surface:

- browser rail:
  exercise list, key selection, support-level badges, and quick filters
- score canvas:
  mirrored score asset with active step emphasis
- guidance area:
  keyboard diagram, finger labels, and compact movement hints
- practice sidebar:
  tempo target, armed MIDI device, capture controls, bookmark state, and short
  user notes
- review panel:
  hidden by default and opened after a completed take to show metrics, weak
  steps, and optional AI advice

## Main States

- browse:
  score and guidance visible, capture idle, review panel hidden
- armed:
  device selected, ready to start a take, capture health visible
- capturing:
  score remains primary, only lightweight take status is shown
- review:
  completed take summary and weak-step analysis visible, optional `Generate
  Advice` action available

## Screen Behavior

- selecting a new exercise updates the score, notes, available keys, and
  support level
- selecting a key switches to the corresponding display asset and overlay set
- step navigation advances the active cursor through the current overlay
- hand mode can simplify guidance to left hand, right hand, or both if the
  chosen overlay provides it
- arming a MIDI device should not start capture by itself
- stopping a take should transition directly into a review-ready state
- AI advice generation is always explicit and never starts in the background

## Practice Flow

1. Open the app and restore the last viewed variant.
2. Confirm the active key variant and target tempo.
3. Arm a MIDI input device.
4. Follow the score and guidance surfaces while preparing the take.
5. Start capture and play through the selected passage or exercise.
6. Stop capture and wait for local pitch/timing analysis to complete.
7. Inspect the review panel for metrics and weak-step summaries.
8. Optionally run `Generate Advice` to request local Codex feedback.
9. Adjust the target tempo, notes, or selected passage and retry.

## Score Presentation

- the score remains the primary visual anchor during browse and capture states
- cursor emphasis must be readable without obscuring notation
- capture controls and device status must not cover the staff image
- practice notes and review summaries must stay out of the score canvas
- missing-overlay or unsupported-analysis states should be obvious, not silent

## Guidance Presentation

The guidance area still combines two roles:

- keyboard help:
  current target region on a piano layout
- fingering help:
  finger labels and simple hand cues tied to the active step

The guidance surface should remain useful even when scored analysis is
unavailable for the current variant.

## Review Panel

The review panel is the only place where analysis and AI output appear.

It should present, in order:

- take identity and status
- aggregate pitch and timing metrics
- weak steps or phrases that need repetition
- notes about degraded analysis if coverage is partial
- optional AI advice card with generated drills or practice suggestions

The live capture surface should never depend on Codex output to stay usable.

## Interaction Model

The app should support both mouse and keyboard interaction.

Recommended defaults:

- mouse for exercise and key selection
- arrow-style step navigation
- visible controls for `Arm`, `Start Take`, `Stop Take`, and `Generate Advice`
- one visible control for hand mode changes
- shortcuts kept modest and documented inside the UI

## Non-Goals

- a full desktop widget suite
- live chat-style coaching during capture
- editable overlays from inside the practice screen
- multi-window workflow
- background AI calls without explicit user action

## Review Notes

The main UI question is no longer whether the screen can show only a score and a
guide. It is whether the app can add capture and review surfaces without
breaking the score-first practice posture.
