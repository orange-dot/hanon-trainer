#ifndef HANON_TRAINER_OVERLAY_STORE_H
#define HANON_TRAINER_OVERLAY_STORE_H

#include <stddef.h>

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a read-only overlay store from corpus_root.
 *
 * Missing overlay TSV files produce an empty store, not an error. On success,
 * stores a non-null handle and transfers ownership to the caller.
 */
ht_status ht_overlay_store_open(ht_overlay_store** out_store, char const* corpus_root);

/**
 * Release an overlay store opened by ht_overlay_store_open.
 *
 * Passing NULL is allowed.
 */
void ht_overlay_store_close(ht_overlay_store* store);

/**
 * Look up overlay metadata by borrowed NUL-terminated overlay_id.
 *
 * On success, out_overlay is fully initialized. On failure, out_overlay is
 * zeroed when non-null.
 */
ht_status ht_overlay_store_get_overlay(ht_overlay_store const* store,
                                       char const* overlay_id,
                                       ht_overlay_record* out_overlay);

/**
 * Look up one overlay step by borrowed overlay_id and zero-based step_index.
 *
 * On success, out_step is fully initialized. On failure, out_step is zeroed
 * when non-null.
 */
ht_status ht_overlay_store_get_step(ht_overlay_store const* store,
                                    char const* overlay_id,
                                    size_t step_index,
                                    ht_overlay_step_record* out_step);

#ifdef __cplusplus
}
#endif

#endif
