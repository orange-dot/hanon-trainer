#include <assert.h>
#include <string.h>

#include <hanon_trainer/overlay_store.h>

int main(void) {
    ht_overlay_store* store = NULL;
    ht_overlay_record overlay;
    ht_overlay_step_record step;

    assert(ht_overlay_store_open(&store, HT_SOURCE_DIR "/corpus") == HT_OK);
    assert(store != NULL);
    assert(ht_overlay_store_get_overlay(store, "fixture-overlay-001", &overlay) == HT_ERR_NOT_FOUND);
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
