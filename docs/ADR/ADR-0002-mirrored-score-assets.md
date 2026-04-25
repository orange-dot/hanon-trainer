# ADR-0002: Mirrored Image-First Score Assets

## Status

Accepted

## Context

The source corpus already provides score imagery and PDFs. Rebuilding notation
rendering in `C` would add large scope and distract from the practice tool
itself.

The runtime also needs a stable coordinate basis for score overlays and
post-session weak-step highlighting.

## Decision

Use mirrored score assets as the base display surface and normalize them into
one image-first runtime asset per variant.

Allow offline PDF-to-raster generation when the mirrored score image is not the
best display source.

Do not build a notation engine in the first build.

## Consequences

- score fidelity comes from source-derived assets, not runtime engraving
- overlay metadata becomes a first-class asset family
- the renderer and analysis overlay system share one stable coordinate space
- PDF rendering does not become a hard runtime dependency
