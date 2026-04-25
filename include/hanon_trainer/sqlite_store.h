#ifndef HANON_TRAINER_SQLITE_STORE_H
#define HANON_TRAINER_SQLITE_STORE_H

#include <stddef.h>

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a SQLite database at database_path.
 *
 * Use ":memory:" for an in-memory database. On success, out_db receives
 * ownership of a non-null handle.
 */
ht_status ht_db_open(ht_db** out_db, char const* database_path);

/**
 * Close a database handle.
 *
 * Passing NULL is allowed.
 */
void ht_db_close(ht_db* db);

/**
 * Apply the local schema migration.
 */
ht_status ht_db_migrate(ht_db* db);

/**
 * Load the single persisted user-state record.
 *
 * If no state exists, this returns HT_OK with a zeroed out_state.
 */
ht_status ht_db_load_user_state(ht_db* db, ht_user_state_record* out_state);

/**
 * Save the single persisted user-state record.
 */
ht_status ht_db_save_user_state(ht_db* db, ht_user_state_record const* state);

/**
 * Insert or replace one practice session record.
 */
ht_status ht_db_insert_session(ht_db* db, ht_session_record const* session);

/**
 * Load a practice session by borrowed NUL-terminated session_id.
 */
ht_status ht_db_load_session(ht_db* db,
                             char const* session_id,
                             ht_session_record* out_session);

/**
 * Append one captured MIDI event.
 */
ht_status ht_db_append_midi_event(ht_db* db, ht_midi_event_record const* event);

/**
 * Load MIDI events for a session sorted by sequence_no.
 *
 * capacity is an element count. If capacity is too small, partial output is
 * allowed and out_count reports the number of available events.
 */
ht_status ht_db_load_midi_events(ht_db* db,
                                 char const* session_id,
                                 ht_midi_event_record* out_events,
                                 size_t capacity,
                                 size_t* out_count);

/**
 * Store aggregate analysis for a completed session.
 */
ht_status ht_db_store_analysis(ht_db* db, ht_analysis_record const* analysis);

/**
 * Load aggregate analysis for a completed session.
 */
ht_status ht_db_load_analysis(ht_db* db,
                              char const* session_id,
                              ht_analysis_record* out_analysis);

/**
 * Store one step-level analysis result.
 */
ht_status ht_db_store_analysis_step(ht_db* db,
                                    ht_analysis_step_record const* step);

/**
 * Load step-level analysis records sorted by step_index.
 *
 * capacity is an element count. If capacity is too small, partial output is
 * allowed and out_count reports the number of stored step results.
 */
ht_status ht_db_load_analysis_steps(ht_db* db,
                                    char const* session_id,
                                    ht_analysis_step_record* out_steps,
                                    size_t capacity,
                                    size_t* out_count);

/**
 * Store one advice artifact.
 */
ht_status ht_db_store_advice(ht_db* db, ht_advice_record const* advice);

#ifdef __cplusplus
}
#endif

#endif
