#ifndef HANON_TRAINER_ADVICE_ORCHESTRATOR_H
#define HANON_TRAINER_ADVICE_ORCHESTRATOR_H

#include <hanon_trainer/codex_cli_adapter.h>
#include <hanon_trainer/content_catalog.h>
#include <hanon_trainer/sqlite_store.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build a typed advice request from persisted local session facts.
 *
 * On success, out_request is fully initialized. On failure, out_request is
 * zeroed when non-null.
 */
ht_status ht_advice_build_request(ht_db* db,
                                  ht_catalog const* catalog,
                                  char const* session_id,
                                  ht_advice_request_record* out_request);

/**
 * Generate advice for a session.
 *
 * This slice is adapter-stubbed: it returns HT_OK, stores an advice artifact,
 * and marks generation_status as HT_GENERATION_STUBBED.
 */
ht_status ht_advice_generate_for_session(ht_db* db,
                                         ht_catalog const* catalog,
                                         ht_codex_cli* cli,
                                         char const* session_id,
                                         ht_advice_record* out_advice);

#ifdef __cplusplus
}
#endif

#endif
