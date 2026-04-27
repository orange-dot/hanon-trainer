#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <hanon_trainer/performance_analysis.h>

static void insert_session(ht_db* db, char const* session_id) {
    ht_session_record session = {0};

    snprintf(session.session_id, sizeof(session.session_id), "%s", session_id);
    snprintf(session.variant_id, sizeof(session.variant_id), "%s", "fixture-variant-pilot");
    snprintf(session.midi_device_id, sizeof(session.midi_device_id), "%s", "fixture-midi");
    snprintf(session.started_at, sizeof(session.started_at), "%s", "start");
    snprintf(session.ended_at, sizeof(session.ended_at), "%s", "end");
    session.target_tempo = 72u;
    session.duration_ms = 2000u;
    session.capture_status = HT_CAPTURE_COMPLETED;
    session.analysis_status = HT_ANALYSIS_PENDING;
    assert(ht_db_insert_session(db, &session) == HT_OK);
}

static void insert_note(ht_db* db,
                        char const* session_id,
                        unsigned sequence_no,
                        int64_t event_ns,
                        unsigned pitch) {
    ht_midi_event_record event = {0};

    snprintf(event.session_id, sizeof(event.session_id), "%s", session_id);
    event.sequence_no = sequence_no;
    event.event_ns = event_ns;
    event.status_byte = 0x90u;
    event.data_1 = pitch;
    event.data_2 = 100u;
    event.channel = 1u;
    event.event_kind = HT_MIDI_EVENT_NOTE_ON;
    assert(ht_db_append_midi_event(db, &event) == HT_OK);
}

static void analyze_case(ht_catalog* catalog,
                         ht_overlay_store* overlays,
                         char const* session_id,
                         unsigned first_pitch,
                         int64_t first_time,
                         bool include_second,
                         int64_t second_time,
                         ht_analysis_record* out_analysis) {
    ht_db* db = NULL;

    assert(ht_db_open(&db, ":memory:") == HT_OK);
    assert(ht_db_migrate(db) == HT_OK);
    insert_session(db, session_id);
    insert_note(db, session_id, 1u, first_time, first_pitch);
    if (include_second) {
        insert_note(db, session_id, 2u, second_time, 62u);
    }
    assert(ht_analysis_run_session(db, catalog, overlays, session_id, out_analysis) == HT_OK);
    ht_db_close(db);
}

int main(void) {
    ht_catalog* catalog = NULL;
    ht_overlay_store* overlays = NULL;
    ht_analysis_record analysis;

    assert(ht_catalog_open(&catalog, HT_SOURCE_DIR "/tests/fixtures/synthetic-corpus") == HT_OK);
    assert(ht_overlay_store_open(&overlays, HT_SOURCE_DIR "/tests/fixtures/synthetic-corpus")
           == HT_OK);

    analyze_case(catalog, overlays, "perfect-session", 60u, 0, true, 1000000000, &analysis);
    assert(analysis.wrong_note_count == 0u);
    assert(analysis.missed_note_count == 0u);
    assert(analysis.extra_note_count == 0u);
    assert(analysis.weak_step_count == 0u);

    analyze_case(catalog, overlays, "wrong-note-session", 61u, 0, true, 1000000000, &analysis);
    assert(analysis.wrong_note_count == 1u);
    assert(analysis.missed_note_count == 0u);
    assert(analysis.weak_step_count == 1u);

    analyze_case(catalog, overlays, "missing-note-session", 60u, 0, false, 0, &analysis);
    assert(analysis.missed_note_count == 1u);
    assert(analysis.weak_step_count == 1u);

    analyze_case(catalog, overlays, "late-note-session", 60u, 0, true, 1120000000, &analysis);
    assert(analysis.max_late_ms == 120);
    assert(analysis.weak_step_count == 1u);

    analyze_case(catalog, overlays, "wrong-late-same-step-session", 61u, 120000000, true, 1000000000, &analysis);
    assert(analysis.wrong_note_count == 1u);
    assert(analysis.max_late_ms == 120);
    assert(analysis.weak_step_count == 1u);

    analyze_case(catalog, overlays, "midpoint-ownership-session", 60u, 490000000, true, 510000000, &analysis);
    assert(analysis.wrong_note_count == 0u);
    assert(analysis.missed_note_count == 0u);
    assert(analysis.extra_note_count == 0u);
    assert(analysis.max_late_ms == 490);
    assert(analysis.max_early_ms == 490);
    assert(analysis.weak_step_count == 2u);

    ht_overlay_store_close(overlays);
    ht_catalog_close(catalog);
    return 0;
}
