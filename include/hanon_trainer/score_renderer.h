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

typedef struct ht_score_renderer_view_request {
    char corpus_root[HT_PATH_CAPACITY];
    char asset_root[HT_PATH_CAPACITY];
    char variant_id[HT_ID_CAPACITY];
    size_t step_index;
    unsigned viewport_width_px;
    unsigned viewport_height_px;
    char output_path[HT_PATH_CAPACITY];
} ht_score_renderer_view_request;

typedef struct ht_score_renderer_view_result {
    char variant_id[HT_ID_CAPACITY];
    char overlay_id[HT_ID_CAPACITY];
    char resolved_asset_path[HT_PATH_CAPACITY];
    unsigned source_asset_width_px;
    unsigned source_asset_height_px;
    unsigned viewport_width_px;
    unsigned viewport_height_px;
    ht_rect_record fitted_score_rect;
    ht_rect_record active_overlay_rect;
    ht_status catalog_status;
    ht_status overlay_status;
    ht_status asset_status;
    ht_status render_status;
} ht_score_renderer_view_result;

/**
 * Render or project one score view for a variant step.
 *
 * The request keeps corpus_root and asset_root separate. corpus_root is passed
 * to the TSV catalog and overlay stores; asset_root resolves catalog asset
 * paths. Empty output_path performs catalog, overlay, layout, and projection
 * only. On success or diagnosable missing content, out_result contains stage
 * statuses and projected geometry.
 */
ht_status ht_score_renderer_render_view(ht_score_renderer* renderer,
                                        ht_score_renderer_view_request const* request,
                                        ht_score_renderer_view_result* out_result);

#ifdef __cplusplus
}
#endif

#endif
