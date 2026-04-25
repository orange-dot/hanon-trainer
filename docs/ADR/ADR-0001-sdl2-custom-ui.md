# ADR-0001: SDL2 Custom UI

## Status

Accepted

## Context

The app needs a Linux-native desktop surface that can combine:

- a score canvas
- lightweight keyboard guidance
- capture controls and device state
- a post-session review panel

A generic widget stack is less important than direct control over rendering,
layout transitions, and input flow.

## Decision

Use `SDL2` as the primary UI and rendering foundation.

Use supporting text and image libraries only as needed for text, icons, and
score-asset display.

Keep the visible UI surface intentionally custom and task-focused rather than
trying to mimic a conventional GTK utility.

## Consequences

- the app keeps a simple native dependency posture
- score presentation, capture controls, and review panels can share one
  rendering model
- desktop widgets remain minimal by design
- text density and annotation density need to be reviewed in practice, but this
  is not a blocker for the first build
