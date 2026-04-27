#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hanon_trainer/performance_analysis.h>

typedef struct fixture_expectation {
    char const* name;
    char const* session_id;
    unsigned wrong_note_count;
    unsigned missed_note_count;
    unsigned extra_note_count;
    int max_late_ms;
    int max_early_ms;
    unsigned weak_step_count;
    bool has_focus_step;
    size_t focus_step;
    ht_pitch_status focus_pitch_status;
    ht_timing_status focus_timing_status;
} fixture_expectation;

static char* trim_line_end(char* line) {
    size_t length;

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

static unsigned parse_unsigned(char const* text) {
    char* end = NULL;
    unsigned long value;

    assert(text != NULL);
    assert(text[0] != '\0');
    errno = 0;
    value = strtoul(text, &end, 10);
    assert(errno == 0);
    assert(end != text);
    assert(*end == '\0');
    assert(value <= 4294967295ul);
    return (unsigned)value;
}

static int64_t parse_i64(char const* text) {
    char* end = NULL;
    long long value;

    assert(text != NULL);
    assert(text[0] != '\0');
    errno = 0;
    value = strtoll(text, &end, 10);
    assert(errno == 0);
    assert(end != text);
    assert(*end == '\0');
    return (int64_t)value;
}

static void insert_session(ht_db* db, char const* session_id) {
    ht_session_record session = {0};

    snprintf(session.session_id, sizeof(session.session_id), "%s", session_id);
    snprintf(session.variant_id, sizeof(session.variant_id), "%s", "hanon-01-c");
    snprintf(session.midi_device_id, sizeof(session.midi_device_id), "%s", "fixture-midi");
    snprintf(session.started_at, sizeof(session.started_at), "%s", "start");
    snprintf(session.ended_at, sizeof(session.ended_at), "%s", "end");
    session.target_tempo = 72u;
    session.duration_ms = 8000u;
    session.capture_status = HT_CAPTURE_COMPLETED;
    session.analysis_status = HT_ANALYSIS_PENDING;
    assert(ht_db_insert_session(db, &session) == HT_OK);
}

static void append_fixture_event(ht_db* db, char* line, char const* session_id) {
    char* field[8];
    char* cursor = line;
    size_t index = 0u;
    ht_midi_event_record event = {0};

    while ((index < 8u) && (cursor != NULL)) {
        field[index] = split_next_field(&cursor, '\t');
        ++index;
    }
    assert(index == 8u);
    assert(cursor == NULL);
    assert(strcmp(field[0], session_id) == 0);

    snprintf(event.session_id, sizeof(event.session_id), "%s", session_id);
    event.sequence_no = parse_unsigned(field[1]);
    event.event_ns = parse_i64(field[2]);
    event.status_byte = parse_unsigned(field[3]);
    event.data_1 = parse_unsigned(field[4]);
    event.data_2 = parse_unsigned(field[5]);
    event.channel = parse_unsigned(field[6]);
    event.event_kind = (ht_midi_event_kind)parse_unsigned(field[7]);
    assert(ht_db_append_midi_event(db, &event) == HT_OK);
}

static void load_fixture_events(ht_db* db, char const* fixture_name, char const* session_id) {
    char path[512];
    char line[1024];
    FILE* file;

    snprintf(path,
             sizeof(path),
             "%s/tests/fixtures/hanon-01-c/%s.tsv",
             HT_SOURCE_DIR,
             fixture_name);
    file = fopen(path, "r");
    assert(file != NULL);

    assert(fgets(line, sizeof(line), file) != NULL);
    assert(strcmp(trim_line_end(line),
                  "session_id\tsequence_no\tevent_ns\tstatus_byte\tdata_1\tdata_2\tchannel\tevent_kind")
           == 0);

    while (fgets(line, sizeof(line), file) != NULL) {
        trim_line_end(line);
        if (line[0] == '\0') {
            continue;
        }
        append_fixture_event(db, line, session_id);
    }
    fclose(file);
}

static void assert_step_statuses(ht_analysis_step_record const steps[8],
                                 fixture_expectation const* expectation) {
    size_t index;

    for (index = 0u; index < 8u; ++index) {
        assert(steps[index].step_index == index);
        if (expectation->has_focus_step && (index == expectation->focus_step)) {
            assert(steps[index].pitch_status == expectation->focus_pitch_status);
            assert(steps[index].timing_status == expectation->focus_timing_status);
        } else {
            assert(steps[index].pitch_status == HT_PITCH_MATCH);
            assert(steps[index].timing_status == HT_TIMING_ON_TIME);
        }
    }
}

static void run_fixture_case(ht_catalog* catalog,
                             ht_overlay_store* overlays,
                             fixture_expectation const* expectation) {
    ht_db* db = NULL;
    ht_analysis_record analysis;
    ht_analysis_record loaded_analysis;
    ht_analysis_step_record steps[8];
    size_t step_count = 0u;

    assert(ht_db_open(&db, ":memory:") == HT_OK);
    assert(ht_db_migrate(db) == HT_OK);
    insert_session(db, expectation->session_id);
    load_fixture_events(db, expectation->name, expectation->session_id);

    assert(ht_analysis_run_session(db, catalog, overlays, expectation->session_id, &analysis)
           == HT_OK);
    assert(ht_db_load_analysis(db, expectation->session_id, &loaded_analysis) == HT_OK);
    assert(loaded_analysis.wrong_note_count == expectation->wrong_note_count);
    assert(loaded_analysis.missed_note_count == expectation->missed_note_count);
    assert(loaded_analysis.extra_note_count == expectation->extra_note_count);
    assert(loaded_analysis.max_late_ms == expectation->max_late_ms);
    assert(loaded_analysis.max_early_ms == expectation->max_early_ms);
    assert(loaded_analysis.weak_step_count == expectation->weak_step_count);
    assert(ht_db_load_analysis_steps(db, expectation->session_id, steps, 8u, &step_count)
           == HT_OK);
    assert(step_count == 8u);
    assert_step_statuses(steps, expectation);

    ht_db_close(db);
}

int main(void) {
    ht_catalog* catalog = NULL;
    ht_overlay_store* overlays = NULL;
    fixture_expectation cases[] = {
        {"perfect", "hanon-01-c-perfect", 0u, 0u, 0u, 0, 0, 0u, false, 0u, 0, 0},
        {"wrong-note",
         "hanon-01-c-wrong-note",
         1u,
         0u,
         0u,
         0,
         0,
         1u,
         true,
         2u,
         HT_PITCH_WRONG,
         HT_TIMING_ON_TIME},
        {"missing-note",
         "hanon-01-c-missing-note",
         0u,
         1u,
         0u,
         0,
         0,
         1u,
         true,
         3u,
         HT_PITCH_MISSED,
         HT_TIMING_ON_TIME},
        {"late-onset",
         "hanon-01-c-late-onset",
         0u,
         0u,
         0u,
         120,
         0,
         1u,
         true,
         5u,
         HT_PITCH_MATCH,
         HT_TIMING_LATE},
    };
    size_t index;

    assert(ht_catalog_open(&catalog, HT_SOURCE_DIR "/corpus") == HT_OK);
    assert(ht_overlay_store_open(&overlays, HT_SOURCE_DIR "/corpus") == HT_OK);

    for (index = 0u; index < (sizeof(cases) / sizeof(cases[0])); ++index) {
        run_fixture_case(catalog, overlays, &cases[index]);
    }

    ht_overlay_store_close(overlays);
    ht_catalog_close(catalog);
    return 0;
}
