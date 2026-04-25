#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <hanon_trainer/sqlite_store.h>

static ht_session_record make_session(char const* session_id) {
    ht_session_record session = {0};

    snprintf(session.session_id, sizeof(session.session_id), "%s", session_id);
    snprintf(session.variant_id, sizeof(session.variant_id), "%s", "fixture-variant-pilot");
    snprintf(session.midi_device_id, sizeof(session.midi_device_id), "%s", "fixture-midi");
    snprintf(session.started_at, sizeof(session.started_at), "%s", "2026-04-25T00:00:00Z");
    snprintf(session.ended_at, sizeof(session.ended_at), "%s", "2026-04-25T00:00:02Z");
    session.target_tempo = 72u;
    session.duration_ms = 2000u;
    session.capture_status = HT_CAPTURE_COMPLETED;
    session.analysis_status = HT_ANALYSIS_PENDING;
    return session;
}

static void assert_foreign_keys_enabled_after_reopen(void) {
    char database_path[256];
    ht_db* db = NULL;
    ht_midi_event_record orphan_event = {0};

    snprintf(database_path, sizeof(database_path), "%s", "/tmp/hanon-trainer-fk-test.sqlite");
    remove(database_path);

    assert(ht_db_open(&db, database_path) == HT_OK);
    assert(ht_db_migrate(db) == HT_OK);
    ht_db_close(db);

    assert(ht_db_open(&db, database_path) == HT_OK);
    snprintf(orphan_event.session_id, sizeof(orphan_event.session_id), "%s", "missing-session");
    orphan_event.sequence_no = 1u;
    orphan_event.event_kind = HT_MIDI_EVENT_NOTE_ON;
    assert(ht_db_append_midi_event(db, &orphan_event) == HT_ERR_DB);
    ht_db_close(db);

    remove(database_path);
}

int main(void) {
    ht_db* db = NULL;
    ht_user_state_record state = {0};
    ht_user_state_record loaded_state;
    ht_session_record session = make_session("sqlite-session");
    ht_session_record loaded_session;
    ht_midi_event_record event = {0};
    ht_midi_event_record events[2];
    ht_analysis_record analysis = {0};
    ht_analysis_record loaded_analysis;
    ht_analysis_step_record step = {0};
    ht_analysis_step_record steps[2];
    ht_advice_record advice = {0};
    size_t count = 0u;

    assert(ht_db_open(&db, ":memory:") == HT_OK);
    assert(ht_db_migrate(db) == HT_OK);
    assert(ht_db_load_user_state(db, &loaded_state) == HT_OK);
    assert(loaded_state.last_variant_id[0] == '\0');

    snprintf(state.last_variant_id, sizeof(state.last_variant_id), "%s", "hanon-01-c");
    state.last_step_index = 3u;
    state.target_tempo = 80u;
    state.bookmark_flag = true;
    snprintf(state.user_note, sizeof(state.user_note), "%s", "private");
    snprintf(state.last_midi_device_id, sizeof(state.last_midi_device_id), "%s", "midi-1");
    snprintf(state.updated_at, sizeof(state.updated_at), "%s", "now");
    assert(ht_db_save_user_state(db, &state) == HT_OK);
    assert(ht_db_load_user_state(db, &loaded_state) == HT_OK);
    assert(strcmp(loaded_state.last_variant_id, "hanon-01-c") == 0);
    assert(loaded_state.bookmark_flag);

    assert(ht_db_insert_session(db, &session) == HT_OK);
    assert(ht_db_load_session(db, "sqlite-session", &loaded_session) == HT_OK);
    assert(strcmp(loaded_session.variant_id, "fixture-variant-pilot") == 0);

    snprintf(event.session_id, sizeof(event.session_id), "%s", "sqlite-session");
    event.sequence_no = 1u;
    event.event_ns = 0;
    event.status_byte = 0x90u;
    event.data_1 = 60u;
    event.data_2 = 100u;
    event.channel = 1u;
    event.event_kind = HT_MIDI_EVENT_NOTE_ON;
    assert(ht_db_append_midi_event(db, &event) == HT_OK);
    event.sequence_no = 2u;
    event.event_ns = 1000000000;
    event.data_1 = 62u;
    assert(ht_db_append_midi_event(db, &event) == HT_OK);
    assert(ht_db_load_midi_events(db, "sqlite-session", events, 1u, &count)
           == HT_ERR_BUFFER_TOO_SMALL);
    assert(count == 2u);
    assert(ht_db_load_midi_events(db, "sqlite-session", events, 2u, &count) == HT_OK);
    assert(count == 2u);
    assert(events[1].data_1 == 62u);

    snprintf(analysis.session_id, sizeof(analysis.session_id), "%s", "sqlite-session");
    snprintf(analysis.analyzed_at, sizeof(analysis.analyzed_at), "%s", "now");
    analysis.weak_step_count = 1u;
    snprintf(analysis.summary_text, sizeof(analysis.summary_text), "%s", "summary");
    assert(ht_db_store_analysis(db, &analysis) == HT_OK);
    assert(ht_db_load_analysis(db, "sqlite-session", &loaded_analysis) == HT_OK);
    assert(loaded_analysis.weak_step_count == 1u);

    snprintf(step.session_id, sizeof(step.session_id), "%s", "sqlite-session");
    step.step_index = 0u;
    step.pitch_status = HT_PITCH_MATCH;
    step.timing_status = HT_TIMING_ON_TIME;
    snprintf(step.note_summary, sizeof(step.note_summary), "%s", "match");
    assert(ht_db_store_analysis_step(db, &step) == HT_OK);
    step.step_index = 1u;
    step.pitch_status = HT_PITCH_WRONG;
    assert(ht_db_store_analysis_step(db, &step) == HT_OK);
    assert(ht_db_load_analysis_steps(db, "sqlite-session", steps, 1u, &count)
           == HT_ERR_BUFFER_TOO_SMALL);
    assert(count == 2u);
    assert(ht_db_load_analysis_steps(db, "sqlite-session", steps, 2u, &count) == HT_OK);
    assert(steps[1].pitch_status == HT_PITCH_WRONG);

    snprintf(advice.advice_id, sizeof(advice.advice_id), "%s", "advice-1");
    snprintf(advice.session_id, sizeof(advice.session_id), "%s", "sqlite-session");
    snprintf(advice.created_at, sizeof(advice.created_at), "%s", "now");
    snprintf(advice.prompt_version, sizeof(advice.prompt_version), "%s", "v1");
    snprintf(advice.codex_command, sizeof(advice.codex_command), "%s", "codex");
    snprintf(advice.request_summary, sizeof(advice.request_summary), "%s", "request");
    snprintf(advice.advice_markdown, sizeof(advice.advice_markdown), "%s", "advice");
    advice.generation_status = HT_GENERATION_STUBBED;
    assert(ht_db_store_advice(db, &advice) == HT_OK);

    ht_db_close(db);
    assert_foreign_keys_enabled_after_reopen();
    return 0;
}
