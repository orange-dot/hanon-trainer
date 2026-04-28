#include <assert.h>
#include <string.h>

#include <hanon_trainer/content_catalog.h>

int main(void) {
    ht_catalog* catalog = NULL;
    ht_variant_record variant;

    assert(ht_catalog_open(&catalog, HT_SOURCE_DIR "/corpus") == HT_OK);
    assert(catalog != NULL);
    assert(ht_catalog_lookup_variant(catalog, "hanon-01-c", &variant) == HT_OK);
    assert(strcmp(variant.exercise_id, "hanon-01") == 0);
    assert(strcmp(variant.variant_id, "hanon-01-c") == 0);
    assert(variant.exercise_number == 1u);
    assert(strcmp(variant.key_name, "C") == 0);
    assert(variant.display_asset_width_px == 2480u);
    assert(variant.display_asset_height_px == 3508u);
    assert(strcmp(variant.overlay_id, "hanon-01-c-pilot-001") == 0);
    assert(variant.support_level == HT_SUPPORT_PILOT_ANALYSIS);
    assert(ht_catalog_lookup_variant(catalog, "missing", &variant) == HT_ERR_NOT_FOUND);
    ht_catalog_close(catalog);

    assert(ht_catalog_open(&catalog, HT_SOURCE_DIR "/tests/fixtures/synthetic-corpus") == HT_OK);
    assert(ht_catalog_lookup_variant(catalog, "fixture-variant-pilot", &variant) == HT_OK);
    assert(strcmp(variant.overlay_id, "fixture-overlay-001") == 0);
    assert(variant.support_level == HT_SUPPORT_PILOT_ANALYSIS);
    ht_catalog_close(catalog);

    return 0;
}
