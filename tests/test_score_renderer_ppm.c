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
                         char const* output_path) {
    memset(request, 0, sizeof(*request));
    set_text(request->corpus_root,
             sizeof(request->corpus_root),
             HT_SOURCE_DIR "/tests/fixtures/viewer-corpus");
    set_text(request->asset_root,
             sizeof(request->asset_root),
             HT_SOURCE_DIR "/tests/fixtures/viewer-corpus");
    set_text(request->variant_id, sizeof(request->variant_id), variant_id);
    set_text(request->output_path, sizeof(request->output_path), output_path);
    request->step_index = 0u;
    request->viewport_width_px = 8u;
    request->viewport_height_px = 8u;
}

static unsigned char const* ppm_raster_start(unsigned char const* data, size_t size) {
    size_t index;
    unsigned newline_count = 0u;

    for (index = 0u; index < size; ++index) {
        if (data[index] == '\n') {
            ++newline_count;
            if (newline_count == 3u) {
                return &data[index + 1u];
            }
        }
    }
    return NULL;
}

static void assert_red_overlay_pixel(char const* path, ht_rect_record rect) {
    FILE* file = fopen(path, "rb");
    unsigned char buffer[512];
    size_t size;
    unsigned char const* raster;
    size_t offset;

    assert(file != NULL);
    size = fread(buffer, 1u, sizeof(buffer), file);
    assert(fclose(file) == 0);
    assert(size > 0u);
    raster = ppm_raster_start(buffer, size);
    assert(raster != NULL);
    offset = (((size_t)rect.y * 8u) + (size_t)rect.x) * 3u;
    assert((&raster[offset + 2u]) < (&buffer[size]));
    assert(raster[offset] == 255u);
    assert(raster[offset + 1u] == 0u);
    assert(raster[offset + 2u] == 0u);
}

static void run_valid(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-valid", "viewer-valid-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
    assert(result.catalog_status == HT_OK);
    assert(result.overlay_status == HT_OK);
    assert(result.asset_status == HT_OK);
    assert(result.render_status == HT_OK);
    assert_red_overlay_pixel("viewer-valid-output.ppm", result.active_overlay_rect);
}

static void run_missing_asset(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-missing-asset", "viewer-missing-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_OK);
    assert(result.asset_status == HT_ERR_NOT_FOUND);
    assert(result.render_status == HT_ERR_NOT_FOUND);
}

static void run_bad_magic(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-bad-magic", "viewer-bad-magic-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_ERR_UNSUPPORTED);
    assert(result.asset_status == HT_ERR_UNSUPPORTED);
}

static void run_bad_max(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-bad-max", "viewer-bad-max-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_ERR_UNSUPPORTED);
    assert(result.asset_status == HT_ERR_UNSUPPORTED);
}

static void run_short_payload(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-short-payload", "viewer-short-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_ERR_CORRUPT_DATA);
    assert(result.asset_status == HT_ERR_CORRUPT_DATA);
}

static void run_malformed_header(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-malformed-header", "viewer-malformed-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_ERR_CORRUPT_DATA);
    assert(result.asset_status == HT_ERR_CORRUPT_DATA);
}

static void run_bad_overlay_dims(ht_score_renderer* renderer) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;

    make_request(&request, "viewer-bad-overlay-dims", "viewer-bad-dims-output.ppm");
    assert(ht_score_renderer_render_view(renderer, &request, &result) == HT_ERR_CORRUPT_DATA);
    assert(result.overlay_status == HT_ERR_CORRUPT_DATA);
}

int main(int argc, char** argv) {
    ht_score_renderer* renderer = NULL;

    assert(argc == 2);
    assert(ht_score_renderer_create(&renderer) == HT_OK);

    if (strcmp(argv[1], "valid") == 0) {
        run_valid(renderer);
    } else if (strcmp(argv[1], "missing-asset") == 0) {
        run_missing_asset(renderer);
    } else if (strcmp(argv[1], "bad-magic") == 0) {
        run_bad_magic(renderer);
    } else if (strcmp(argv[1], "bad-max") == 0) {
        run_bad_max(renderer);
    } else if (strcmp(argv[1], "short-payload") == 0) {
        run_short_payload(renderer);
    } else if (strcmp(argv[1], "malformed-header") == 0) {
        run_malformed_header(renderer);
    } else if (strcmp(argv[1], "bad-overlay-dims") == 0) {
        run_bad_overlay_dims(renderer);
    } else {
        assert(false);
    }

    ht_score_renderer_destroy(renderer);
    return 0;
}
