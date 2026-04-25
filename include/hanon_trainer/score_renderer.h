#ifndef HANON_TRAINER_SCORE_RENDERER_H
#define HANON_TRAINER_SCORE_RENDERER_H

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a score renderer handle.
 *
 * This compile-boundary slice does not open a window or initialize SDL video.
 */
ht_status ht_score_renderer_create(ht_score_renderer** out_renderer);

/**
 * Release a score renderer handle.
 *
 * Passing NULL is allowed.
 */
void ht_score_renderer_destroy(ht_score_renderer* renderer);

/**
 * Probe linked SDL2 and SDL_ttf versions without opening a window.
 */
ht_status ht_score_renderer_probe(ht_score_renderer* renderer);

#ifdef __cplusplus
}
#endif

#endif
