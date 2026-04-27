#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <hanon_trainer/score_renderer.h>

static void set_text(char* destination, size_t capacity, char const* source) {
    int written = snprintf(destination, capacity, "%s", source);

    assert(written >= 0);
    assert((size_t)written < capacity);
}

static void make_request(ht_score_renderer_view_request* request, size_t step_index) {
    memset(request, 0, sizeof(*request));
    set_text(request->corpus_root, sizeof(request->corpus_root), HT_SOURCE_DIR "/corpus");
    set_text(request->asset_root, sizeof(request->asset_root), HT_SOURCE_DIR);
    set_text(request->variant_id, sizeof(request->variant_id), "hanon-01-c");
    request->step_index = step_index;
    request->viewport_width_px = 1280u;
    request->viewport_height_px = 900u;
}

static void assert_rect_inside(ht_rect_record inner, ht_rect_record outer) {
    assert(inner.x >= outer.x);
    assert(inner.y >= outer.y);
    assert((inner.x + inner.width) <= (outer.x + outer.width));
    assert((inner.y + inner.height) <= (outer.y + outer.height));
    assert(inner.width > 0);
    assert(inner.height > 0);
}

int main(void) {
    ht_score_renderer* renderer = NULL;
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;
    int first_step_x = 0;
    int last_step_x = 0;
    size_t index;

    assert(ht_score_renderer_create(&renderer) == HT_OK);

    for (index = 0u; index < 8u; ++index) {
        make_request(&request, index);
        assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
        assert(strcmp(result.variant_id, "hanon-01-c") == 0);
        assert(strcmp(result.overlay_id, "hanon-01-c-pilot-001") == 0);
        assert(result.catalog_status == HT_OK);
        assert(result.overlay_status == HT_OK);
        assert(result.render_status == HT_OK);
        assert(result.source_asset_width_px == 2480u);
        assert(result.source_asset_height_px == 3508u);
        assert(result.fitted_score_rect.x == 322);
        assert(result.fitted_score_rect.y == 0);
        assert(result.fitted_score_rect.width == 636);
        assert(result.fitted_score_rect.height == 900);
        assert_rect_inside(result.active_overlay_rect, result.fitted_score_rect);
        if (index == 0u) {
            first_step_x = result.active_overlay_rect.x;
        }
        if (index == 7u) {
            last_step_x = result.active_overlay_rect.x;
        }
    }

    assert(first_step_x < last_step_x);

    make_request(&request, 8u);
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
    assert(result.overlay_status == HT_ERR_NOT_FOUND);
    assert(result.render_status == HT_ERR_NOT_FOUND);

    ht_score_renderer_destroy(renderer);
    return 0;
}
