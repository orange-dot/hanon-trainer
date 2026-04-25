#include <hanon_trainer/content_catalog.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ht_catalog {
    ht_variant_record* variants;
    size_t variant_count;
};

static void zero_variant(ht_variant_record* out_variant) {
    if (out_variant != NULL) {
        memset(out_variant, 0, sizeof(*out_variant));
    }
}

static ht_status copy_text(char* destination, size_t capacity, char const* source) {
    size_t length;

    if ((destination == NULL) || (capacity == 0u) || (source == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    length = strlen(source);
    if (length >= capacity) {
        destination[0] = '\0';
        return HT_ERR_BUFFER_TOO_SMALL;
    }

    memcpy(destination, source, length + 1u);
    return HT_OK;
}

static ht_status join_path(char* destination,
                           size_t capacity,
                           char const* root,
                           char const* relative_path) {
    int written;

    if ((destination == NULL) || (root == NULL) || (relative_path == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    written = snprintf(destination, capacity, "%s/%s", root, relative_path);
    if ((written < 0) || ((size_t)written >= capacity)) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }

    return HT_OK;
}

static char* trim_line_end(char* line) {
    size_t length;

    if (line == NULL) {
        return NULL;
    }

    length = strlen(line);
    while ((length > 0u) && ((line[length - 1u] == '\n') || (line[length - 1u] == '\r'))) {
        line[length - 1u] = '\0';
        --length;
    }

    return line;
}

static ht_status parse_unsigned(char const* text, unsigned* out_value) {
    char* end = NULL;
    unsigned long value;

    if ((text == NULL) || (out_value == NULL) || (text[0] == '\0') || (text[0] == '-')) {
        return HT_ERR_CORRUPT_DATA;
    }

    errno = 0;
    value = strtoul(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0') || (value > 4294967295ul)) {
        return HT_ERR_CORRUPT_DATA;
    }

    *out_value = (unsigned)value;
    return HT_OK;
}

static ht_status parse_variant_line(char* line, ht_variant_record* out_variant) {
    char* field[16];
    size_t index = 0u;
    char* cursor = line;
    ht_status status;

    if ((line == NULL) || (out_variant == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_variant, 0, sizeof(*out_variant));
    while ((index < 16u) && (cursor != NULL)) {
        field[index] = strsep(&cursor, "\t");
        ++index;
    }
    if (index != 16u) {
        return HT_ERR_CORRUPT_DATA;
    }

    status = copy_text(out_variant->exercise_id, sizeof(out_variant->exercise_id), field[0]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->variant_id, sizeof(out_variant->variant_id), field[1]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_unsigned(field[2], &out_variant->exercise_number);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->title, sizeof(out_variant->title), field[3]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->key_name, sizeof(out_variant->key_name), field[4]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_unsigned(field[5], &out_variant->sort_order);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->tempo_text, sizeof(out_variant->tempo_text), field[6]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->repeat_text, sizeof(out_variant->repeat_text), field[7]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->practice_notes, sizeof(out_variant->practice_notes), field[8]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->source_page_path, sizeof(out_variant->source_page_path), field[9]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->source_pdf_path, sizeof(out_variant->source_pdf_path), field[10]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->display_asset_path, sizeof(out_variant->display_asset_path), field[11]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_unsigned(field[12], &out_variant->display_asset_width_px);
    if (status != HT_OK) {
        return status;
    }
    status = parse_unsigned(field[13], &out_variant->display_asset_height_px);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_variant->overlay_id, sizeof(out_variant->overlay_id), field[14]);
    if (status != HT_OK) {
        return status;
    }

    return ht_support_level_from_string(field[15], &out_variant->support_level);
}

ht_status ht_catalog_open(ht_catalog** out_catalog, char const* corpus_root) {
    char path[HT_PATH_CAPACITY];
    FILE* file = NULL;
    char line[4096];
    size_t capacity = 0u;
    ht_catalog* catalog = NULL;

    if (out_catalog != NULL) {
        *out_catalog = NULL;
    }
    if ((out_catalog == NULL) || (corpus_root == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (join_path(path, sizeof(path), corpus_root, "catalog/variants.tsv") != HT_OK) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }

    file = fopen(path, "r");
    if (file == NULL) {
        return HT_ERR_IO;
    }

    catalog = calloc(1u, sizeof(*catalog));
    if (catalog == NULL) {
        fclose(file);
        return HT_ERR_IO;
    }

    if (fgets(line, sizeof(line), file) == NULL) {
        ht_catalog_close(catalog);
        fclose(file);
        return HT_ERR_CORRUPT_DATA;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        ht_variant_record parsed;
        ht_variant_record* next_variants;
        ht_status status;

        trim_line_end(line);
        if (line[0] == '\0') {
            continue;
        }

        status = parse_variant_line(line, &parsed);
        if (status != HT_OK) {
            ht_catalog_close(catalog);
            fclose(file);
            return status;
        }

        if (catalog->variant_count == capacity) {
            size_t next_capacity = (capacity == 0u) ? 8u : capacity * 2u;
            next_variants = realloc(catalog->variants, next_capacity * sizeof(*next_variants));
            if (next_variants == NULL) {
                ht_catalog_close(catalog);
                fclose(file);
                return HT_ERR_IO;
            }
            catalog->variants = next_variants;
            capacity = next_capacity;
        }

        catalog->variants[catalog->variant_count] = parsed;
        ++catalog->variant_count;
    }

    fclose(file);
    *out_catalog = catalog;
    return HT_OK;
}

void ht_catalog_close(ht_catalog* catalog) {
    if (catalog == NULL) {
        return;
    }

    free(catalog->variants);
    free(catalog);
}

ht_status ht_catalog_lookup_variant(ht_catalog const* catalog,
                                    char const* variant_id,
                                    ht_variant_record* out_variant) {
    size_t index;

    zero_variant(out_variant);
    if ((catalog == NULL) || (variant_id == NULL) || (out_variant == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    for (index = 0u; index < catalog->variant_count; ++index) {
        if (strcmp(catalog->variants[index].variant_id, variant_id) == 0) {
            *out_variant = catalog->variants[index];
            return HT_OK;
        }
    }

    return HT_ERR_NOT_FOUND;
}
