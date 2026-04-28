#include <hanon_trainer/performance_analysis.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    HT_STEP_NS = 1000000000
};

typedef struct note_group_score {
    size_t candidate_count;
    size_t matched_count;
    int64_t earliest_event_ns;
    int earliest_error_ms;
    bool has_candidate;
} note_group_score;

static int64_t expected_step_time_ns(size_t step_index) {
    return (int64_t)step_index * (int64_t)HT_STEP_NS;
}

static int64_t step_lower_bound_ns(size_t step_index) {
    if (step_index == 0u) {
        return 0;
    }
    return expected_step_time_ns(step_index) - ((int64_t)HT_STEP_NS / 2);
}

static int64_t step_upper_bound_ns(size_t step_index) {
    return expected_step_time_ns(step_index) + ((int64_t)HT_STEP_NS / 2);
}

static bool note_on_owned_by_step(ht_midi_event_record const* event, size_t step_index) {
    int64_t lower;
    int64_t upper;

    if ((event == NULL) || (event->event_kind != HT_MIDI_EVENT_NOTE_ON)) {
        return false;
    }

    lower = step_lower_bound_ns(step_index);
    upper = step_upper_bound_ns(step_index);
    return (event->event_ns >= lower) && (event->event_ns < upper);
}

static bool note_on_owned_by_any_step(ht_midi_event_record const* event, size_t step_count) {
    if ((event == NULL) || (event->event_kind != HT_MIDI_EVENT_NOTE_ON)) {
        return false;
    }
    if (event->event_ns < 0) {
        return false;
    }
    return event->event_ns < step_upper_bound_ns(step_count - 1u);
}

static void score_step_group(ht_overlay_step_record const* step,
                             ht_midi_event_record const* events,
                             size_t event_count,
                             note_group_score* out_score) {
    bool matched_expected[HT_MAX_EXPECTED_PITCHES] = {false};
    size_t index;

    memset(out_score, 0, sizeof(*out_score));
    out_score->earliest_error_ms = 0;

    for (index = 0u; index < event_count; ++index) {
        size_t expected_index;
        int event_error_ms;

        if (!note_on_owned_by_step(&events[index], step->step_index)) {
            continue;
        }

        ++out_score->candidate_count;
        event_error_ms =
            (int)((events[index].event_ns - expected_step_time_ns(step->step_index)) / 1000000);
        if (!out_score->has_candidate || (events[index].event_ns < out_score->earliest_event_ns)) {
            out_score->earliest_event_ns = events[index].event_ns;
            out_score->earliest_error_ms = event_error_ms;
            out_score->has_candidate = true;
        }

        for (expected_index = 0u; expected_index < step->expected_pitch_count; ++expected_index) {
            if (!matched_expected[expected_index]
                && (step->expected_pitches[expected_index] == (int)events[index].data_1)) {
                matched_expected[expected_index] = true;
                ++out_score->matched_count;
                break;
            }
        }
    }
}

static size_t min_size(size_t left, size_t right) {
    return (left < right) ? left : right;
}

static size_t count_extra_note_on_events(ht_midi_event_record const* events,
                                         size_t event_count,
                                         size_t step_count) {
    size_t index;
    size_t count = 0u;

    if (step_count == 0u) {
        return 0u;
    }
    for (index = 0u; index < event_count; ++index) {
        if ((events[index].event_kind == HT_MIDI_EVENT_NOTE_ON)
            && !note_on_owned_by_any_step(&events[index], step_count)) {
            ++count;
        }
    }
    return count;
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
        note_group_score group;
        size_t expected_count;
        size_t comparable_count;
        bool step_was_weak = false;

        status = ht_overlay_store_get_step(overlays, overlay.overlay_id, step_index, &step);
        if (status != HT_OK) {
            free(events);
            return status;
        }

        memset(&step_result, 0, sizeof(step_result));
        set_text(step_result.session_id, sizeof(step_result.session_id), session_id);
        step_result.step_index = step_index;
        score_step_group(&step, events, event_count, &group);

        expected_count = step.expected_pitch_count;
        comparable_count = min_size(group.candidate_count, expected_count);

        if (group.matched_count < comparable_count) {
            analysis.wrong_note_count += (unsigned)(comparable_count - group.matched_count);
        }
        if (group.candidate_count < expected_count) {
            analysis.missed_note_count += (unsigned)(expected_count - group.candidate_count);
        }
        if (group.candidate_count > expected_count) {
            analysis.extra_note_count += (unsigned)(group.candidate_count - expected_count);
        }

        if (group.candidate_count < expected_count) {
            step_was_weak = true;
            step_result.pitch_status = HT_PITCH_MISSED;
        } else if ((group.matched_count == expected_count)
                   && (group.candidate_count == expected_count)) {
            step_result.pitch_status = HT_PITCH_MATCH;
        } else {
            step_was_weak = true;
            step_result.pitch_status = HT_PITCH_WRONG;
        }

        if (!group.has_candidate) {
            step_was_weak = true;
            step_result.timing_status = HT_TIMING_MISSING;
            set_text(step_result.note_summary, sizeof(step_result.note_summary), "missing note");
        } else {
            if (abs_int(group.earliest_error_ms) <= step.timing_window_ms) {
                step_result.timing_status = HT_TIMING_ON_TIME;
            } else if (group.earliest_error_ms < 0) {
                step_was_weak = true;
                step_result.timing_status = HT_TIMING_EARLY;
                if (abs_int(group.earliest_error_ms) > analysis.max_early_ms) {
                    analysis.max_early_ms = abs_int(group.earliest_error_ms);
                }
            } else {
                step_was_weak = true;
                step_result.timing_status = HT_TIMING_LATE;
                if (group.earliest_error_ms > analysis.max_late_ms) {
                    analysis.max_late_ms = group.earliest_error_ms;
                }
            }

            total_abs_error_ms += abs_int(group.earliest_error_ms);
            ++timed_event_count;
            if ((step_result.pitch_status == HT_PITCH_MATCH)
                && (step_result.timing_status == HT_TIMING_ON_TIME)) {
                set_text(step_result.note_summary, sizeof(step_result.note_summary), "match");
            } else {
                set_text(step_result.note_summary, sizeof(step_result.note_summary), "review step");
            }
        }
        if (step_was_weak) {
            ++analysis.weak_step_count;
        }

        status = ht_db_store_analysis_step(db, &step_result);
        if (status != HT_OK) {
            free(events);
            return status;
        }
    }

    analysis.extra_note_count +=
        (unsigned)count_extra_note_on_events(events, event_count, overlay.step_count);
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
