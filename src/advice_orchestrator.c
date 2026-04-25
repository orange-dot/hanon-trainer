#include <hanon_trainer/advice_orchestrator.h>

#include <stdio.h>
#include <string.h>

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

ht_status ht_advice_build_request(ht_db* db,
                                  ht_catalog const* catalog,
                                  char const* session_id,
                                  ht_advice_request_record* out_request) {
    ht_session_record session;
    ht_variant_record variant;
    ht_analysis_record analysis;
    ht_status status;

    if (out_request != NULL) {
        memset(out_request, 0, sizeof(*out_request));
    }
    if ((db == NULL) || (catalog == NULL) || (session_id == NULL) || (out_request == NULL)) {
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

    set_text(out_request->session_id, sizeof(out_request->session_id), session.session_id);
    set_text(out_request->variant_id, sizeof(out_request->variant_id), variant.variant_id);
    out_request->exercise_number = variant.exercise_number;
    set_text(out_request->exercise_title, sizeof(out_request->exercise_title), variant.title);
    set_text(out_request->key_name, sizeof(out_request->key_name), variant.key_name);
    out_request->target_tempo = session.target_tempo;
    out_request->support_level = variant.support_level;

    status = ht_db_load_analysis(db, session_id, &analysis);
    if (status == HT_OK) {
        snprintf(out_request->pitch_summary,
                 sizeof(out_request->pitch_summary),
                 "wrong=%u missed=%u extra=%u",
                 analysis.wrong_note_count,
                 analysis.missed_note_count,
                 analysis.extra_note_count);
        snprintf(out_request->timing_summary,
                 sizeof(out_request->timing_summary),
                 "mean_abs_ms=%.2f max_late_ms=%d max_early_ms=%d",
                 analysis.mean_onset_error_ms,
                 analysis.max_late_ms,
                 analysis.max_early_ms);
        snprintf(out_request->weak_step_summary,
                 sizeof(out_request->weak_step_summary),
                 "weak_steps=%u",
                 analysis.weak_step_count);
    } else if (status == HT_ERR_NOT_FOUND) {
        set_text(out_request->pitch_summary, sizeof(out_request->pitch_summary), "analysis unavailable");
        set_text(out_request->timing_summary, sizeof(out_request->timing_summary), "analysis unavailable");
        set_text(out_request->weak_step_summary, sizeof(out_request->weak_step_summary), "analysis unavailable");
        status = HT_OK;
    }

    return status;
}

ht_status ht_advice_generate_for_session(ht_db* db,
                                         ht_catalog const* catalog,
                                         ht_codex_cli* cli,
                                         char const* session_id,
                                         ht_advice_record* out_advice) {
    ht_advice_request_record request;
    ht_advice_record advice;
    ht_status status;

    (void)cli;
    if (out_advice != NULL) {
        memset(out_advice, 0, sizeof(*out_advice));
    }
    if ((db == NULL) || (catalog == NULL) || (session_id == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = ht_advice_build_request(db, catalog, session_id, &request);
    if (status != HT_OK) {
        return status;
    }

    memset(&advice, 0, sizeof(advice));
    snprintf(advice.advice_id, sizeof(advice.advice_id), "%s-stub-advice", session_id);
    set_text(advice.session_id, sizeof(advice.session_id), session_id);
    set_text(advice.created_at, sizeof(advice.created_at), "synthetic-now");
    set_text(advice.prompt_version, sizeof(advice.prompt_version), "stub-v1");
    set_text(advice.codex_command, sizeof(advice.codex_command), "codex-cli-not-invoked");
    snprintf(advice.request_summary,
             sizeof(advice.request_summary),
             "variant=%s generation=stubbed",
             request.variant_id);
    set_text(advice.advice_markdown,
             sizeof(advice.advice_markdown),
             "Advice generation is stubbed until manual input is reviewed.");
    advice.generation_status = HT_GENERATION_STUBBED;

    status = ht_db_store_advice(db, &advice);
    if (status != HT_OK) {
        return status;
    }
    if (out_advice != NULL) {
        *out_advice = advice;
    }
    return HT_OK;
}
