#include <hanon_trainer/overlay_store.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ht_overlay_store {
    ht_overlay_record* overlays;
    size_t overlay_count;
    size_t overlay_capacity;
    ht_overlay_step_record* steps;
    size_t step_count;
    size_t step_capacity;
};

static void zero_overlay(ht_overlay_record* out_overlay) {
    if (out_overlay != NULL) {
        memset(out_overlay, 0, sizeof(*out_overlay));
    }
}

static void zero_step(ht_overlay_step_record* out_step) {
    if (out_step != NULL) {
        memset(out_step, 0, sizeof(*out_step));
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

static char* split_next_field(char** cursor, char delimiter) {
    char* field;
    char* split;

    if ((cursor == NULL) || (*cursor == NULL)) {
        return NULL;
    }
    field = *cursor;
    split = strchr(field, delimiter);
    if (split == NULL) {
        *cursor = NULL;
    } else {
        *split = '\0';
        *cursor = split + 1;
    }
    return field;
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

static ht_status parse_size(char const* text, size_t* out_value) {
    char* end = NULL;
    unsigned long value;

    if ((text == NULL) || (out_value == NULL) || (text[0] == '\0') || (text[0] == '-')) {
        return HT_ERR_CORRUPT_DATA;
    }
    errno = 0;
    value = strtoul(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0')) {
        return HT_ERR_CORRUPT_DATA;
    }
    *out_value = (size_t)value;
    return HT_OK;
}

static ht_status parse_int(char const* text, int* out_value) {
    char* end = NULL;
    long value;

    if ((text == NULL) || (out_value == NULL) || (text[0] == '\0')) {
        return HT_ERR_CORRUPT_DATA;
    }
    errno = 0;
    value = strtol(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0')) {
        return HT_ERR_CORRUPT_DATA;
    }
    *out_value = (int)value;
    return HT_OK;
}

static ht_status parse_bool(char const* text, bool* out_value) {
    if ((text == NULL) || (out_value == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (strcmp(text, "true") == 0) {
        *out_value = true;
        return HT_OK;
    }
    if (strcmp(text, "false") == 0) {
        *out_value = false;
        return HT_OK;
    }
    return HT_ERR_CORRUPT_DATA;
}

static ht_status parse_pitches(char const* text,
                               int out_pitches[HT_MAX_EXPECTED_PITCHES],
                               size_t* out_count) {
    char buffer[HT_TEXT_CAPACITY];
    char* cursor = buffer;
    char* token;
    size_t count = 0u;
    ht_status status;

    if ((text == NULL) || (out_pitches == NULL) || (out_count == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (copy_text(buffer, sizeof(buffer), text) != HT_OK) {
        return HT_ERR_CORRUPT_DATA;
    }

    if (buffer[0] == '\0') {
        *out_count = 0u;
        return HT_OK;
    }

    while ((token = split_next_field(&cursor, ' ')) != NULL) {
        if (token[0] == '\0') {
            continue;
        }
        if (count >= HT_MAX_EXPECTED_PITCHES) {
            return HT_ERR_BUFFER_TOO_SMALL;
        }
        status = parse_int(token, &out_pitches[count]);
        if (status != HT_OK) {
            return status;
        }
        ++count;
    }

    *out_count = count;
    return HT_OK;
}

static ht_status parse_overlay_line(char* line, ht_overlay_record* out_overlay) {
    char* field[8];
    char* cursor = line;
    size_t index = 0u;
    ht_status status;

    if ((line == NULL) || (out_overlay == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_overlay, 0, sizeof(*out_overlay));
    while ((index < 8u) && (cursor != NULL)) {
        field[index] = split_next_field(&cursor, '\t');
        ++index;
    }
    if (index != 8u) {
        return HT_ERR_CORRUPT_DATA;
    }

    status = copy_text(out_overlay->overlay_id, sizeof(out_overlay->overlay_id), field[0]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_overlay->variant_id, sizeof(out_overlay->variant_id), field[1]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_size(field[2], &out_overlay->step_count);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_overlay->hand_scope, sizeof(out_overlay->hand_scope), field[3]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_bool(field[4], &out_overlay->analysis_enabled);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_overlay->coverage_scope, sizeof(out_overlay->coverage_scope), field[5]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_unsigned(field[6], &out_overlay->reference_asset_width_px);
    if (status != HT_OK) {
        return status;
    }
    return parse_unsigned(field[7], &out_overlay->reference_asset_height_px);
}

static ht_status parse_step_line(char* line, ht_overlay_step_record* out_step) {
    char* field[12];
    char* cursor = line;
    size_t index = 0u;
    ht_status status;

    if ((line == NULL) || (out_step == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_step, 0, sizeof(*out_step));
    while ((index < 12u) && (cursor != NULL)) {
        field[index] = split_next_field(&cursor, '\t');
        ++index;
    }
    if (index != 12u) {
        return HT_ERR_CORRUPT_DATA;
    }

    status = copy_text(out_step->overlay_id, sizeof(out_step->overlay_id), field[0]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_size(field[1], &out_step->step_index);
    if (status != HT_OK) {
        return status;
    }
    status = parse_unsigned(field[2], &out_step->page_index);
    if (status != HT_OK) {
        return status;
    }
    status = parse_int(field[3], &out_step->cursor_region.x);
    if (status != HT_OK) {
        return status;
    }
    status = parse_int(field[4], &out_step->cursor_region.y);
    if (status != HT_OK) {
        return status;
    }
    status = parse_int(field[5], &out_step->cursor_region.width);
    if (status != HT_OK) {
        return status;
    }
    status = parse_int(field[6], &out_step->cursor_region.height);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_step->keyboard_target, sizeof(out_step->keyboard_target), field[7]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_step->finger_label, sizeof(out_step->finger_label), field[8]);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_step->step_note, sizeof(out_step->step_note), field[9]);
    if (status != HT_OK) {
        return status;
    }
    status = parse_pitches(field[10], out_step->expected_pitches, &out_step->expected_pitch_count);
    if (status != HT_OK) {
        return status;
    }
    return parse_int(field[11], &out_step->timing_window_ms);
}

static ht_status append_overlay(ht_overlay_store* store, ht_overlay_record const* overlay) {
    ht_overlay_record* next;
    size_t next_capacity;

    if (store->overlay_count == store->overlay_capacity) {
        next_capacity = (store->overlay_capacity == 0u) ? 8u : store->overlay_capacity * 2u;
        next = realloc(store->overlays, next_capacity * sizeof(*next));
        if (next == NULL) {
            return HT_ERR_IO;
        }
        store->overlays = next;
        store->overlay_capacity = next_capacity;
    }
    store->overlays[store->overlay_count] = *overlay;
    ++store->overlay_count;
    return HT_OK;
}

static ht_status append_step(ht_overlay_store* store, ht_overlay_step_record const* step) {
    ht_overlay_step_record* next;
    size_t next_capacity;

    if (store->step_count == store->step_capacity) {
        next_capacity = (store->step_capacity == 0u) ? 16u : store->step_capacity * 2u;
        next = realloc(store->steps, next_capacity * sizeof(*next));
        if (next == NULL) {
            return HT_ERR_IO;
        }
        store->steps = next;
        store->step_capacity = next_capacity;
    }
    store->steps[store->step_count] = *step;
    ++store->step_count;
    return HT_OK;
}

static ht_status load_overlay_metadata(ht_overlay_store* store, char const* corpus_root) {
    char path[HT_PATH_CAPACITY];
    FILE* file;
    char line[4096];
    ht_status status;

    status = join_path(path, sizeof(path), corpus_root, "overlays/overlay_metadata.tsv");
    if (status != HT_OK) {
        return status;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        return HT_OK;
    }
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return HT_ERR_CORRUPT_DATA;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        ht_overlay_record overlay;

        trim_line_end(line);
        if (line[0] == '\0') {
            continue;
        }
        status = parse_overlay_line(line, &overlay);
        if (status != HT_OK) {
            fclose(file);
            return status;
        }
        status = append_overlay(store, &overlay);
        if (status != HT_OK) {
            fclose(file);
            return status;
        }
    }
    fclose(file);
    return HT_OK;
}

static ht_status load_overlay_steps(ht_overlay_store* store, char const* corpus_root) {
    char path[HT_PATH_CAPACITY];
    FILE* file;
    char line[4096];
    ht_status status;

    status = join_path(path, sizeof(path), corpus_root, "overlays/overlay_steps.tsv");
    if (status != HT_OK) {
        return status;
    }
    file = fopen(path, "r");
    if (file == NULL) {
        return HT_OK;
    }
    if (fgets(line, sizeof(line), file) == NULL) {
        fclose(file);
        return HT_ERR_CORRUPT_DATA;
    }
    while (fgets(line, sizeof(line), file) != NULL) {
        ht_overlay_step_record step;

        trim_line_end(line);
        if (line[0] == '\0') {
            continue;
        }
        status = parse_step_line(line, &step);
        if (status != HT_OK) {
            fclose(file);
            return status;
        }
        status = append_step(store, &step);
        if (status != HT_OK) {
            fclose(file);
            return status;
        }
    }
    fclose(file);
    return HT_OK;
}

ht_status ht_overlay_store_open(ht_overlay_store** out_store, char const* corpus_root) {
    ht_overlay_store* store = NULL;
    ht_status status;

    if (out_store != NULL) {
        *out_store = NULL;
    }
    if ((out_store == NULL) || (corpus_root == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    store = calloc(1u, sizeof(*store));
    if (store == NULL) {
        return HT_ERR_IO;
    }

    status = load_overlay_metadata(store, corpus_root);
    if (status == HT_OK) {
        status = load_overlay_steps(store, corpus_root);
    }
    if (status != HT_OK) {
        ht_overlay_store_close(store);
        return status;
    }

    *out_store = store;
    return HT_OK;
}

void ht_overlay_store_close(ht_overlay_store* store) {
    if (store == NULL) {
        return;
    }
    free(store->overlays);
    free(store->steps);
    free(store);
}

ht_status ht_overlay_store_get_overlay(ht_overlay_store const* store,
                                       char const* overlay_id,
                                       ht_overlay_record* out_overlay) {
    size_t index;

    zero_overlay(out_overlay);
    if ((store == NULL) || (overlay_id == NULL) || (out_overlay == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    for (index = 0u; index < store->overlay_count; ++index) {
        if (strcmp(store->overlays[index].overlay_id, overlay_id) == 0) {
            *out_overlay = store->overlays[index];
            return HT_OK;
        }
    }

    return HT_ERR_NOT_FOUND;
}

ht_status ht_overlay_store_get_step(ht_overlay_store const* store,
                                    char const* overlay_id,
                                    size_t step_index,
                                    ht_overlay_step_record* out_step) {
    size_t index;

    zero_step(out_step);
    if ((store == NULL) || (overlay_id == NULL) || (out_step == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    for (index = 0u; index < store->step_count; ++index) {
        if ((strcmp(store->steps[index].overlay_id, overlay_id) == 0)
            && (store->steps[index].step_index == step_index)) {
            *out_step = store->steps[index];
            return HT_OK;
        }
    }

    return HT_ERR_NOT_FOUND;
}
