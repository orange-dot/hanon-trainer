#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <hanon_trainer/score_renderer.h>

static void set_text(char* destination, size_t capacity, char const* source) {
    int written = snprintf(destination, capacity, "%s", source);

    assert(written >= 0);
    assert((size_t)written < capacity);
}

static void make_request(ht_score_renderer_view_request* request,
                         char const* variant_id,
                         size_t step_index,
                         unsigned viewport_width,
                         unsigned viewport_height) {
    memset(request, 0, sizeof(*request));
    set_text(request->corpus_root,
             sizeof(request->corpus_root),
             HT_SOURCE_DIR "/tests/fixtures/viewer-corpus");
    set_text(request->asset_root,
             sizeof(request->asset_root),
             HT_SOURCE_DIR "/tests/fixtures/viewer-corpus");
    set_text(request->variant_id, sizeof(request->variant_id), variant_id);
    request->step_index = step_index;
    request->viewport_width_px = viewport_width;
    request->viewport_height_px = viewport_height;
}

int main(void) {
    ht_score_renderer* renderer = NULL;
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    assert(ht_score_renderer_create(&renderer) == HT_OK);

    make_request(&request, "viewer-valid", 0u, 8u, 8u);
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
    assert(result.catalog_status == HT_OK);
    assert(result.overlay_status == HT_OK);
    assert(result.asset_status == HT_OK);
    assert(result.render_status == HT_OK);
    assert(result.source_asset_width_px == 4u);
    assert(result.source_asset_height_px == 4u);
    assert(result.fitted_score_rect.x == 0);
    assert(result.fitted_score_rect.y == 0);
    assert(result.fitted_score_rect.width == 8);
    assert(result.fitted_score_rect.height == 8);
    assert(result.active_overlay_rect.x == 2);
    assert(result.active_overlay_rect.y == 2);
    assert(result.active_overlay_rect.width == 4);
    assert(result.active_overlay_rect.height == 4);

    make_request(&request, "viewer-valid", 0u, 12u, 8u);
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
    assert(result.fitted_score_rect.x == 2);
    assert(result.fitted_score_rect.y == 0);
    assert(result.fitted_score_rect.width == 8);
    assert(result.fitted_score_rect.height == 8);
    assert(result.active_overlay_rect.x == 4);
    assert(result.active_overlay_rect.y == 2);
    assert(result.active_overlay_rect.width == 4);
    assert(result.active_overlay_rect.height == 4);

    make_request(&request, "viewer-valid", 9u, 8u, 8u);
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
    assert(result.overlay_status == HT_ERR_NOT_FOUND);
    assert(result.render_status == HT_ERR_NOT_FOUND);

    make_request(&request, "viewer-valid", 0u, 0u, 8u);
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_ERR_INVALID_ARG);

    ht_score_renderer_destroy(renderer);
    return 0;
}
