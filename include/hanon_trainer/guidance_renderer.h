#ifndef HANON_TRAINER_GUIDANCE_RENDERER_H
#define HANON_TRAINER_GUIDANCE_RENDERER_H

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a guidance renderer handle.
 *
 * On success, out_renderer receives ownership of a non-null handle.
 */
ht_status ht_guidance_renderer_create(ht_guidance_renderer** out_renderer);

/**
 * Release a guidance renderer handle.
 *
 * Passing NULL is allowed.
 */
void ht_guidance_renderer_destroy(ht_guidance_renderer* renderer);

#ifdef __cplusplus
}
#endif

#endif
