#include <assert.h>
#include <string.h>

#include <hanon_trainer/overlay_store.h>

int main(void) {
    ht_overlay_store* store = NULL;
    ht_overlay_record overlay;
    ht_overlay_step_record step;
    int expected_pitches[8][2] = {
        {48, 60},
        {50, 62},
        {52, 64},
        {53, 65},
        {55, 67},
        {53, 65},
        {52, 64},
        {50, 62},
    };
    size_t index;

    assert(ht_overlay_store_open(&store, HT_SOURCE_DIR "/corpus") == HT_OK);
    assert(store != NULL);
    assert(ht_overlay_store_get_overlay(store, "fixture-overlay-001", &overlay) == HT_ERR_NOT_FOUND);
    assert(ht_overlay_store_get_overlay(store, "hanon-01-c-pilot-001", &overlay) == HT_OK);
    assert(strcmp(overlay.variant_id, "hanon-01-c") == 0);
    assert(overlay.step_count == 8u);
    assert(strcmp(overlay.hand_scope, "both") == 0);
    assert(overlay.analysis_enabled);
    assert(strcmp(overlay.coverage_scope, "bounded_passage") == 0);
    assert(overlay.reference_asset_width_px == 2480u);
    assert(overlay.reference_asset_height_px == 3508u);
    for (index = 0u; index < 8u; ++index) {
        assert(ht_overlay_store_get_step(store, "hanon-01-c-pilot-001", index, &step) == HT_OK);
        assert(step.step_index == index);
        assert(step.page_index == 0u);
        assert(step.cursor_region.x >= 0);
        assert(step.cursor_region.y >= 0);
        assert(step.cursor_region.width > 0);
        assert(step.cursor_region.height > 0);
        assert((unsigned)(step.cursor_region.x + step.cursor_region.width) <= 2480u);
        assert((unsigned)(step.cursor_region.y + step.cursor_region.height) <= 3508u);
        assert(step.expected_pitch_count == 2u);
        assert(step.expected_pitches[0] == expected_pitches[index][0]);
        assert(step.expected_pitches[1] == expected_pitches[index][1]);
        assert(step.timing_window_ms == 80);
    }
    assert(ht_overlay_store_get_step(store, "hanon-01-c-pilot-001", 8u, &step) == HT_ERR_NOT_FOUND);
    ht_overlay_store_close(store);

    assert(ht_overlay_store_open(&store, HT_SOURCE_DIR "/tests/fixtures/synthetic-corpus") == HT_OK);
    assert(ht_overlay_store_get_overlay(store, "fixture-overlay-001", &overlay) == HT_OK);
    assert(strcmp(overlay.variant_id, "fixture-variant-pilot") == 0);
    assert(overlay.step_count == 2u);
    assert(overlay.analysis_enabled);
    assert(ht_overlay_store_get_step(store, "fixture-overlay-001", 0u, &step) == HT_OK);
    assert(step.expected_pitch_count == 1u);
    assert(step.expected_pitches[0] == 60);
    assert(step.timing_window_ms == 50);
    assert(ht_overlay_store_get_step(store, "fixture-overlay-001", 3u, &step) == HT_ERR_NOT_FOUND);
    ht_overlay_store_close(store);

    assert(ht_overlay_store_open(&store, HT_SOURCE_DIR "/tests/fixtures/bad-overlay")
           == HT_ERR_CORRUPT_DATA);
    assert(store == NULL);

    return 0;
}
