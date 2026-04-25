#include <hanon_trainer/performance_analysis.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool pitch_matches(ht_overlay_step_record const* step, unsigned pitch) {
    size_t index;

    for (index = 0u; index < step->expected_pitch_count; ++index) {
        if (step->expected_pitches[index] == (int)pitch) {
            return true;
        }
    }
    return false;
}

static size_t find_next_note_on(ht_midi_event_record const* events,
                                size_t event_count,
                                size_t start_index) {
    size_t index;

    for (index = start_index; index < event_count; ++index) {
        if (events[index].event_kind == HT_MIDI_EVENT_NOTE_ON) {
            return index;
        }
    }
    return event_count;
}

static void set_text(char* destination, size_t capacity, char const* text) {
    size_t length;

    if ((destination == NULL) || (capacity == 0u) || (text == NULL)) {
        return;
    }
    length = strlen(text);
    if (length >= capacity) {
        length = capacity - 1u;
    }
    memcpy(destination, text, length);
    destination[length] = '\0';
}

static int abs_int(int value) {
    return (value < 0) ? -value : value;
}

ht_status ht_analysis_run_session(ht_db* db,
                                  ht_catalog const* catalog,
                                  ht_overlay_store const* overlays,
                                  char const* session_id,
                                  ht_analysis_record* out_analysis) {
    ht_session_record session;
    ht_variant_record variant;
    ht_overlay_record overlay;
    ht_midi_event_record* events = NULL;
    size_t event_count = 0u;
    size_t event_index = 0u;
    size_t step_index;
    unsigned timed_event_count = 0u;
    int total_abs_error_ms = 0;
    ht_analysis_record analysis;
    ht_status status;

    if (out_analysis != NULL) {
        memset(out_analysis, 0, sizeof(*out_analysis));
    }
    if ((db == NULL) || (catalog == NULL) || (overlays == NULL) || (session_id == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = ht_db_load_session(db, session_id, &session);
    if (status != HT_OK) {
        return status;
    }
    status = ht_catalog_lookup_variant(catalog, session.variant_id, &variant);
    if (status != HT_OK) {
        return status;
    }
    if (variant.overlay_id[0] == '\0') {
        return HT_ERR_UNSUPPORTED;
    }
    status = ht_overlay_store_get_overlay(overlays, variant.overlay_id, &overlay);
    if (status != HT_OK) {
        return status;
    }
    if (!overlay.analysis_enabled) {
        return HT_ERR_UNSUPPORTED;
    }

    status = ht_db_load_midi_events(db, session_id, NULL, 0u, &event_count);
    if ((status != HT_OK) && (status != HT_ERR_BUFFER_TOO_SMALL)) {
        return status;
    }
    if (event_count > 0u) {
        events = calloc(event_count, sizeof(*events));
        if (events == NULL) {
            return HT_ERR_IO;
        }
        status = ht_db_load_midi_events(db, session_id, events, event_count, &event_count);
        if (status != HT_OK) {
            free(events);
            return status;
        }
    }

    memset(&analysis, 0, sizeof(analysis));
    set_text(analysis.session_id, sizeof(analysis.session_id), session_id);
    set_text(analysis.analyzed_at, sizeof(analysis.analyzed_at), "synthetic-now");

    for (step_index = 0u; step_index < overlay.step_count; ++step_index) {
        ht_overlay_step_record step;
        ht_analysis_step_record step_result;
        size_t matched_index;

        status = ht_overlay_store_get_step(overlays, overlay.overlay_id, step_index, &step);
        if (status != HT_OK) {
            free(events);
            return status;
        }

        memset(&step_result, 0, sizeof(step_result));
        set_text(step_result.session_id, sizeof(step_result.session_id), session_id);
        step_result.step_index = step_index;
        matched_index = find_next_note_on(events, event_count, event_index);
        if (matched_index == event_count) {
            ++analysis.missed_note_count;
            ++analysis.weak_step_count;
            step_result.pitch_status = HT_PITCH_MISSED;
            step_result.timing_status = HT_TIMING_MISSING;
            set_text(step_result.note_summary, sizeof(step_result.note_summary), "missing note");
        } else {
            ht_midi_event_record const* event = &events[matched_index];
            int expected_ms = (int)step_index * 1000;
            int actual_ms = (int)(event->event_ns / 1000000);
            int error_ms = actual_ms - expected_ms;

            event_index = matched_index + 1u;
            if (pitch_matches(&step, event->data_1)) {
                step_result.pitch_status = HT_PITCH_MATCH;
            } else {
                ++analysis.wrong_note_count;
                ++analysis.weak_step_count;
                step_result.pitch_status = HT_PITCH_WRONG;
            }

            if (abs_int(error_ms) <= step.timing_window_ms) {
                step_result.timing_status = HT_TIMING_ON_TIME;
            } else if (error_ms < 0) {
                ++analysis.weak_step_count;
                step_result.timing_status = HT_TIMING_EARLY;
                if (abs_int(error_ms) > analysis.max_early_ms) {
                    analysis.max_early_ms = abs_int(error_ms);
                }
            } else {
                ++analysis.weak_step_count;
                step_result.timing_status = HT_TIMING_LATE;
                if (error_ms > analysis.max_late_ms) {
                    analysis.max_late_ms = error_ms;
                }
            }

            total_abs_error_ms += abs_int(error_ms);
            ++timed_event_count;
            if ((step_result.pitch_status == HT_PITCH_MATCH)
                && (step_result.timing_status == HT_TIMING_ON_TIME)) {
                set_text(step_result.note_summary, sizeof(step_result.note_summary), "match");
            } else {
                set_text(step_result.note_summary, sizeof(step_result.note_summary), "review step");
            }
        }

        status = ht_db_store_analysis_step(db, &step_result);
        if (status != HT_OK) {
            free(events);
            return status;
        }
    }

    while (find_next_note_on(events, event_count, event_index) != event_count) {
        event_index = find_next_note_on(events, event_count, event_index) + 1u;
        ++analysis.extra_note_count;
    }
    if (timed_event_count > 0u) {
        analysis.mean_onset_error_ms = (double)total_abs_error_ms / (double)timed_event_count;
    }

    snprintf(analysis.summary_text,
             sizeof(analysis.summary_text),
             "wrong=%u missed=%u extra=%u weak=%u",
             analysis.wrong_note_count,
             analysis.missed_note_count,
             analysis.extra_note_count,
             analysis.weak_step_count);

    status = ht_db_store_analysis(db, &analysis);
    free(events);
    if (status != HT_OK) {
        return status;
    }
    if (out_analysis != NULL) {
        *out_analysis = analysis;
    }
    return HT_OK;
}
