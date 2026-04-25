#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <hanon_trainer/advice_orchestrator.h>

int main(void) {
    ht_catalog* catalog = NULL;
    ht_db* db = NULL;
    ht_codex_cli* cli = NULL;
    ht_session_record session = {0};
    ht_analysis_record analysis = {0};
    ht_advice_request_record request;
    ht_advice_record advice;

    assert(ht_catalog_open(&catalog, HT_SOURCE_DIR "/tests/fixtures/synthetic-corpus") == HT_OK);
    assert(ht_db_open(&db, ":memory:") == HT_OK);
    assert(ht_db_migrate(db) == HT_OK);
    assert(ht_codex_cli_open(&cli, "codex", NULL, 30000u) == HT_OK);

    snprintf(session.session_id, sizeof(session.session_id), "%s", "advice-session");
    snprintf(session.variant_id, sizeof(session.variant_id), "%s", "fixture-variant-pilot");
    snprintf(session.midi_device_id, sizeof(session.midi_device_id), "%s", "fixture-midi");
    snprintf(session.started_at, sizeof(session.started_at), "%s", "start");
    snprintf(session.ended_at, sizeof(session.ended_at), "%s", "end");
    session.target_tempo = 72u;
    session.duration_ms = 1000u;
    session.capture_status = HT_CAPTURE_COMPLETED;
    session.analysis_status = HT_ANALYSIS_COMPLETE;
    assert(ht_db_insert_session(db, &session) == HT_OK);

    snprintf(analysis.session_id, sizeof(analysis.session_id), "%s", "advice-session");
    snprintf(analysis.analyzed_at, sizeof(analysis.analyzed_at), "%s", "now");
    analysis.wrong_note_count = 1u;
    analysis.mean_onset_error_ms = 12.0;
    snprintf(analysis.summary_text, sizeof(analysis.summary_text), "%s", "summary");
    assert(ht_db_store_analysis(db, &analysis) == HT_OK);

    assert(ht_advice_build_request(db, catalog, "advice-session", &request) == HT_OK);
    assert(strcmp(request.variant_id, "fixture-variant-pilot") == 0);
    assert(strstr(request.pitch_summary, "wrong=1") != NULL);

    assert(ht_advice_generate_for_session(db, catalog, cli, "advice-session", &advice) == HT_OK);
    assert(advice.generation_status == HT_GENERATION_STUBBED);
    assert(strstr(advice.advice_markdown, "stubbed") != NULL);

    ht_codex_cli_close(cli);
    ht_db_close(db);
    ht_catalog_close(catalog);
    return 0;
}
