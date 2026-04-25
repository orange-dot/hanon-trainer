#include <hanon_trainer/sqlite_store.h>

#include <sqlite3.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ht_db {
    sqlite3* connection;
};

static void zero_state(ht_user_state_record* out_state) {
    if (out_state != NULL) {
        memset(out_state, 0, sizeof(*out_state));
    }
}

static void zero_session(ht_session_record* out_session) {
    if (out_session != NULL) {
        memset(out_session, 0, sizeof(*out_session));
    }
}

static void zero_analysis(ht_analysis_record* out_analysis) {
    if (out_analysis != NULL) {
        memset(out_analysis, 0, sizeof(*out_analysis));
    }
}

static ht_status copy_text(char* destination, size_t capacity, unsigned char const* source) {
    size_t length;

    if ((destination == NULL) || (capacity == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    destination[0] = '\0';
    if (source == NULL) {
        return HT_OK;
    }

    length = strlen((char const*)source);
    if (length >= capacity) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(destination, source, length + 1u);
    return HT_OK;
}

static ht_status prepare_statement(ht_db* db, char const* sql, sqlite3_stmt** out_statement) {
    if ((db == NULL) || (db->connection == NULL) || (sql == NULL) || (out_statement == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (sqlite3_prepare_v2(db->connection, sql, -1, out_statement, NULL) != SQLITE_OK) {
        return HT_ERR_DB;
    }
    return HT_OK;
}

static ht_status step_done(sqlite3_stmt* statement) {
    int rc = sqlite3_step(statement);

    if (rc == SQLITE_DONE) {
        return HT_OK;
    }
    if (rc == SQLITE_ROW) {
        return HT_OK;
    }
    return HT_ERR_DB;
}

static ht_status bind_text(sqlite3_stmt* statement, int index, char const* text) {
    if ((statement == NULL) || (text == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (sqlite3_bind_text(statement, index, text, -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        return HT_ERR_DB;
    }
    return HT_OK;
}

static ht_status count_rows(ht_db* db, char const* sql, char const* session_id, size_t* out_count) {
    sqlite3_stmt* statement = NULL;
    ht_status status;
    int rc;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((db == NULL) || (sql == NULL) || (session_id == NULL) || (out_count == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db, sql, &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, session_id);
    }
    if (status == HT_OK) {
        rc = sqlite3_step(statement);
        if (rc == SQLITE_ROW) {
            *out_count = (size_t)sqlite3_column_int64(statement, 0);
        } else {
            status = HT_ERR_DB;
        }
    }

    sqlite3_finalize(statement);
    return status;
}

ht_status ht_db_open(ht_db** out_db, char const* database_path) {
    ht_db* db;

    if (out_db != NULL) {
        *out_db = NULL;
    }
    if ((out_db == NULL) || (database_path == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    db = calloc(1u, sizeof(*db));
    if (db == NULL) {
        return HT_ERR_IO;
    }
    if (sqlite3_open(database_path, &db->connection) != SQLITE_OK) {
        ht_db_close(db);
        return HT_ERR_DB;
    }
    if (sqlite3_exec(db->connection, "PRAGMA foreign_keys = ON;", NULL, NULL, NULL) != SQLITE_OK) {
        ht_db_close(db);
        return HT_ERR_DB;
    }

    *out_db = db;
    return HT_OK;
}

void ht_db_close(ht_db* db) {
    if (db == NULL) {
        return;
    }
    if (db->connection != NULL) {
        sqlite3_close(db->connection);
    }
    free(db);
}

ht_status ht_db_migrate(ht_db* db) {
    char const* sql =
        "PRAGMA foreign_keys = ON;"
        "CREATE TABLE IF NOT EXISTS user_state ("
        "singleton_id INTEGER PRIMARY KEY CHECK (singleton_id = 1),"
        "last_variant_id TEXT NOT NULL,"
        "last_step_index INTEGER NOT NULL,"
        "target_tempo INTEGER NOT NULL,"
        "bookmark_flag INTEGER NOT NULL,"
        "user_note TEXT NOT NULL,"
        "last_midi_device_id TEXT NOT NULL,"
        "updated_at TEXT NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS practice_sessions ("
        "session_id TEXT PRIMARY KEY,"
        "variant_id TEXT NOT NULL,"
        "midi_device_id TEXT NOT NULL,"
        "target_tempo INTEGER NOT NULL,"
        "started_at TEXT NOT NULL,"
        "ended_at TEXT NOT NULL,"
        "duration_ms INTEGER NOT NULL,"
        "capture_status INTEGER NOT NULL,"
        "analysis_status INTEGER NOT NULL"
        ");"
        "CREATE TABLE IF NOT EXISTS midi_events ("
        "session_id TEXT NOT NULL,"
        "sequence_no INTEGER NOT NULL,"
        "event_ns INTEGER NOT NULL,"
        "status_byte INTEGER NOT NULL,"
        "data_1 INTEGER NOT NULL,"
        "data_2 INTEGER NOT NULL,"
        "channel INTEGER NOT NULL,"
        "event_kind INTEGER NOT NULL,"
        "PRIMARY KEY (session_id, sequence_no),"
        "FOREIGN KEY (session_id) REFERENCES practice_sessions(session_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS analysis_results ("
        "session_id TEXT PRIMARY KEY,"
        "analyzed_at TEXT NOT NULL,"
        "wrong_note_count INTEGER NOT NULL,"
        "missed_note_count INTEGER NOT NULL,"
        "extra_note_count INTEGER NOT NULL,"
        "mean_onset_error_ms REAL NOT NULL,"
        "max_late_ms INTEGER NOT NULL,"
        "max_early_ms INTEGER NOT NULL,"
        "weak_step_count INTEGER NOT NULL,"
        "summary_text TEXT NOT NULL,"
        "FOREIGN KEY (session_id) REFERENCES practice_sessions(session_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS analysis_step_results ("
        "session_id TEXT NOT NULL,"
        "step_index INTEGER NOT NULL,"
        "pitch_status INTEGER NOT NULL,"
        "timing_status INTEGER NOT NULL,"
        "note_summary TEXT NOT NULL,"
        "PRIMARY KEY (session_id, step_index),"
        "FOREIGN KEY (session_id) REFERENCES practice_sessions(session_id)"
        ");"
        "CREATE TABLE IF NOT EXISTS advice_artifacts ("
        "advice_id TEXT PRIMARY KEY,"
        "session_id TEXT NOT NULL,"
        "created_at TEXT NOT NULL,"
        "prompt_version TEXT NOT NULL,"
        "codex_command TEXT NOT NULL,"
        "request_summary TEXT NOT NULL,"
        "advice_markdown TEXT NOT NULL,"
        "generation_status INTEGER NOT NULL,"
        "FOREIGN KEY (session_id) REFERENCES practice_sessions(session_id)"
        ");";

    if ((db == NULL) || (db->connection == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (sqlite3_exec(db->connection, sql, NULL, NULL, NULL) != SQLITE_OK) {
        return HT_ERR_DB;
    }
    return HT_OK;
}

ht_status ht_db_load_user_state(ht_db* db, ht_user_state_record* out_state) {
    sqlite3_stmt* statement = NULL;
    ht_status status;
    int rc;

    zero_state(out_state);
    if ((db == NULL) || (out_state == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "SELECT last_variant_id,last_step_index,target_tempo,bookmark_flag,"
                               "user_note,last_midi_device_id,updated_at "
                               "FROM user_state WHERE singleton_id = 1",
                               &statement);
    if (status != HT_OK) {
        sqlite3_finalize(statement);
        return status;
    }

    rc = sqlite3_step(statement);
    if (rc == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return HT_OK;
    }
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return HT_ERR_DB;
    }

    status = copy_text(out_state->last_variant_id,
                       sizeof(out_state->last_variant_id),
                       sqlite3_column_text(statement, 0));
    if (status == HT_OK) {
        out_state->last_step_index = (size_t)sqlite3_column_int64(statement, 1);
        out_state->target_tempo = (unsigned)sqlite3_column_int(statement, 2);
        out_state->bookmark_flag = sqlite3_column_int(statement, 3) != 0;
        status = copy_text(out_state->user_note,
                           sizeof(out_state->user_note),
                           sqlite3_column_text(statement, 4));
    }
    if (status == HT_OK) {
        status = copy_text(out_state->last_midi_device_id,
                           sizeof(out_state->last_midi_device_id),
                           sqlite3_column_text(statement, 5));
    }
    if (status == HT_OK) {
        status = copy_text(out_state->updated_at,
                           sizeof(out_state->updated_at),
                           sqlite3_column_text(statement, 6));
    }

    sqlite3_finalize(statement);
    if (status != HT_OK) {
        zero_state(out_state);
    }
    return status;
}

ht_status ht_db_save_user_state(ht_db* db, ht_user_state_record const* state) {
    sqlite3_stmt* statement = NULL;
    ht_status status;

    if ((db == NULL) || (state == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "INSERT OR REPLACE INTO user_state "
                               "(singleton_id,last_variant_id,last_step_index,target_tempo,"
                               "bookmark_flag,user_note,last_midi_device_id,updated_at) "
                               "VALUES (1,?,?,?,?,?,?,?)",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, state->last_variant_id);
    }
    if (status == HT_OK) {
        sqlite3_bind_int64(statement, 2, (sqlite3_int64)state->last_step_index);
        sqlite3_bind_int(statement, 3, (int)state->target_tempo);
        sqlite3_bind_int(statement, 4, state->bookmark_flag ? 1 : 0);
        status = bind_text(statement, 5, state->user_note);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 6, state->last_midi_device_id);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 7, state->updated_at);
    }
    if (status == HT_OK) {
        status = step_done(statement);
    }

    sqlite3_finalize(statement);
    return status;
}

ht_status ht_db_insert_session(ht_db* db, ht_session_record const* session) {
    sqlite3_stmt* statement = NULL;
    ht_status status;

    if ((db == NULL) || (session == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "INSERT OR REPLACE INTO practice_sessions "
                               "(session_id,variant_id,midi_device_id,target_tempo,started_at,ended_at,"
                               "duration_ms,capture_status,analysis_status) "
                               "VALUES (?,?,?,?,?,?,?,?,?)",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, session->session_id);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 2, session->variant_id);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 3, session->midi_device_id);
    }
    if (status == HT_OK) {
        sqlite3_bind_int(statement, 4, (int)session->target_tempo);
        status = bind_text(statement, 5, session->started_at);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 6, session->ended_at);
    }
    if (status == HT_OK) {
        sqlite3_bind_int(statement, 7, (int)session->duration_ms);
        sqlite3_bind_int(statement, 8, (int)session->capture_status);
        sqlite3_bind_int(statement, 9, (int)session->analysis_status);
        status = step_done(statement);
    }

    sqlite3_finalize(statement);
    return status;
}

ht_status ht_db_load_session(ht_db* db,
                             char const* session_id,
                             ht_session_record* out_session) {
    sqlite3_stmt* statement = NULL;
    ht_status status;
    int rc;

    zero_session(out_session);
    if ((db == NULL) || (session_id == NULL) || (out_session == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "SELECT session_id,variant_id,midi_device_id,target_tempo,started_at,"
                               "ended_at,duration_ms,capture_status,analysis_status "
                               "FROM practice_sessions WHERE session_id = ?",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, session_id);
    }
    if (status != HT_OK) {
        sqlite3_finalize(statement);
        return status;
    }

    rc = sqlite3_step(statement);
    if (rc == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return HT_ERR_NOT_FOUND;
    }
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return HT_ERR_DB;
    }

    status = copy_text(out_session->session_id,
                       sizeof(out_session->session_id),
                       sqlite3_column_text(statement, 0));
    if (status == HT_OK) {
        status = copy_text(out_session->variant_id,
                           sizeof(out_session->variant_id),
                           sqlite3_column_text(statement, 1));
    }
    if (status == HT_OK) {
        status = copy_text(out_session->midi_device_id,
                           sizeof(out_session->midi_device_id),
                           sqlite3_column_text(statement, 2));
    }
    if (status == HT_OK) {
        out_session->target_tempo = (unsigned)sqlite3_column_int(statement, 3);
        status = copy_text(out_session->started_at,
                           sizeof(out_session->started_at),
                           sqlite3_column_text(statement, 4));
    }
    if (status == HT_OK) {
        status = copy_text(out_session->ended_at,
                           sizeof(out_session->ended_at),
                           sqlite3_column_text(statement, 5));
    }
    if (status == HT_OK) {
        out_session->duration_ms = (unsigned)sqlite3_column_int(statement, 6);
        out_session->capture_status = (ht_capture_status)sqlite3_column_int(statement, 7);
        out_session->analysis_status = (ht_analysis_status)sqlite3_column_int(statement, 8);
    }

    sqlite3_finalize(statement);
    if (status != HT_OK) {
        zero_session(out_session);
    }
    return status;
}

ht_status ht_db_append_midi_event(ht_db* db, ht_midi_event_record const* event) {
    sqlite3_stmt* statement = NULL;
    ht_status status;

    if ((db == NULL) || (event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "INSERT OR REPLACE INTO midi_events "
                               "(session_id,sequence_no,event_ns,status_byte,data_1,data_2,channel,event_kind) "
                               "VALUES (?,?,?,?,?,?,?,?)",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, event->session_id);
    }
    if (status == HT_OK) {
        sqlite3_bind_int(statement, 2, (int)event->sequence_no);
        sqlite3_bind_int64(statement, 3, (sqlite3_int64)event->event_ns);
        sqlite3_bind_int(statement, 4, (int)event->status_byte);
        sqlite3_bind_int(statement, 5, (int)event->data_1);
        sqlite3_bind_int(statement, 6, (int)event->data_2);
        sqlite3_bind_int(statement, 7, (int)event->channel);
        sqlite3_bind_int(statement, 8, (int)event->event_kind);
        status = step_done(statement);
    }

    sqlite3_finalize(statement);
    return status;
}

ht_status ht_db_load_midi_events(ht_db* db,
                                 char const* session_id,
                                 ht_midi_event_record* out_events,
                                 size_t capacity,
                                 size_t* out_count) {
    sqlite3_stmt* statement = NULL;
    ht_status status;
    size_t available = 0u;
    size_t index = 0u;
    int rc;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((db == NULL) || (session_id == NULL) || (out_count == NULL)
        || ((capacity > 0u) && (out_events == NULL))) {
        return HT_ERR_INVALID_ARG;
    }

    status = count_rows(db,
                        "SELECT COUNT(*) FROM midi_events WHERE session_id = ?",
                        session_id,
                        &available);
    if (status != HT_OK) {
        return status;
    }
    *out_count = available;

    status = prepare_statement(db,
                               "SELECT session_id,sequence_no,event_ns,status_byte,data_1,data_2,channel,event_kind "
                               "FROM midi_events WHERE session_id = ? ORDER BY sequence_no",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, session_id);
    }
    if (status != HT_OK) {
        sqlite3_finalize(statement);
        return status;
    }

    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        if (index < capacity) {
            ht_midi_event_record* event = &out_events[index];
            memset(event, 0, sizeof(*event));
            status = copy_text(event->session_id,
                               sizeof(event->session_id),
                               sqlite3_column_text(statement, 0));
            if (status != HT_OK) {
                sqlite3_finalize(statement);
                return status;
            }
            event->sequence_no = (unsigned)sqlite3_column_int(statement, 1);
            event->event_ns = (int64_t)sqlite3_column_int64(statement, 2);
            event->status_byte = (unsigned)sqlite3_column_int(statement, 3);
            event->data_1 = (unsigned)sqlite3_column_int(statement, 4);
            event->data_2 = (unsigned)sqlite3_column_int(statement, 5);
            event->channel = (unsigned)sqlite3_column_int(statement, 6);
            event->event_kind = (ht_midi_event_kind)sqlite3_column_int(statement, 7);
        }
        ++index;
    }
    sqlite3_finalize(statement);

    if (rc != SQLITE_DONE) {
        return HT_ERR_DB;
    }
    if (available > capacity) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    return HT_OK;
}

ht_status ht_db_store_analysis(ht_db* db, ht_analysis_record const* analysis) {
    sqlite3_stmt* statement = NULL;
    ht_status status;

    if ((db == NULL) || (analysis == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "INSERT OR REPLACE INTO analysis_results "
                               "(session_id,analyzed_at,wrong_note_count,missed_note_count,extra_note_count,"
                               "mean_onset_error_ms,max_late_ms,max_early_ms,weak_step_count,summary_text) "
                               "VALUES (?,?,?,?,?,?,?,?,?,?)",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, analysis->session_id);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 2, analysis->analyzed_at);
    }
    if (status == HT_OK) {
        sqlite3_bind_int(statement, 3, (int)analysis->wrong_note_count);
        sqlite3_bind_int(statement, 4, (int)analysis->missed_note_count);
        sqlite3_bind_int(statement, 5, (int)analysis->extra_note_count);
        sqlite3_bind_double(statement, 6, analysis->mean_onset_error_ms);
        sqlite3_bind_int(statement, 7, analysis->max_late_ms);
        sqlite3_bind_int(statement, 8, analysis->max_early_ms);
        sqlite3_bind_int(statement, 9, (int)analysis->weak_step_count);
        status = bind_text(statement, 10, analysis->summary_text);
    }
    if (status == HT_OK) {
        status = step_done(statement);
    }

    sqlite3_finalize(statement);
    return status;
}

ht_status ht_db_load_analysis(ht_db* db,
                              char const* session_id,
                              ht_analysis_record* out_analysis) {
    sqlite3_stmt* statement = NULL;
    ht_status status;
    int rc;

    zero_analysis(out_analysis);
    if ((db == NULL) || (session_id == NULL) || (out_analysis == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "SELECT session_id,analyzed_at,wrong_note_count,missed_note_count,"
                               "extra_note_count,mean_onset_error_ms,max_late_ms,max_early_ms,"
                               "weak_step_count,summary_text "
                               "FROM analysis_results WHERE session_id = ?",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, session_id);
    }
    if (status != HT_OK) {
        sqlite3_finalize(statement);
        return status;
    }

    rc = sqlite3_step(statement);
    if (rc == SQLITE_DONE) {
        sqlite3_finalize(statement);
        return HT_ERR_NOT_FOUND;
    }
    if (rc != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return HT_ERR_DB;
    }

    status = copy_text(out_analysis->session_id,
                       sizeof(out_analysis->session_id),
                       sqlite3_column_text(statement, 0));
    if (status == HT_OK) {
        status = copy_text(out_analysis->analyzed_at,
                           sizeof(out_analysis->analyzed_at),
                           sqlite3_column_text(statement, 1));
    }
    if (status == HT_OK) {
        out_analysis->wrong_note_count = (unsigned)sqlite3_column_int(statement, 2);
        out_analysis->missed_note_count = (unsigned)sqlite3_column_int(statement, 3);
        out_analysis->extra_note_count = (unsigned)sqlite3_column_int(statement, 4);
        out_analysis->mean_onset_error_ms = sqlite3_column_double(statement, 5);
        out_analysis->max_late_ms = sqlite3_column_int(statement, 6);
        out_analysis->max_early_ms = sqlite3_column_int(statement, 7);
        out_analysis->weak_step_count = (unsigned)sqlite3_column_int(statement, 8);
        status = copy_text(out_analysis->summary_text,
                           sizeof(out_analysis->summary_text),
                           sqlite3_column_text(statement, 9));
    }

    sqlite3_finalize(statement);
    if (status != HT_OK) {
        zero_analysis(out_analysis);
    }
    return status;
}

ht_status ht_db_store_analysis_step(ht_db* db,
                                    ht_analysis_step_record const* step) {
    sqlite3_stmt* statement = NULL;
    ht_status status;

    if ((db == NULL) || (step == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "INSERT OR REPLACE INTO analysis_step_results "
                               "(session_id,step_index,pitch_status,timing_status,note_summary) "
                               "VALUES (?,?,?,?,?)",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, step->session_id);
    }
    if (status == HT_OK) {
        sqlite3_bind_int64(statement, 2, (sqlite3_int64)step->step_index);
        sqlite3_bind_int(statement, 3, (int)step->pitch_status);
        sqlite3_bind_int(statement, 4, (int)step->timing_status);
        status = bind_text(statement, 5, step->note_summary);
    }
    if (status == HT_OK) {
        status = step_done(statement);
    }

    sqlite3_finalize(statement);
    return status;
}

ht_status ht_db_load_analysis_steps(ht_db* db,
                                    char const* session_id,
                                    ht_analysis_step_record* out_steps,
                                    size_t capacity,
                                    size_t* out_count) {
    sqlite3_stmt* statement = NULL;
    ht_status status;
    size_t available = 0u;
    size_t index = 0u;
    int rc;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((db == NULL) || (session_id == NULL) || (out_count == NULL)
        || ((capacity > 0u) && (out_steps == NULL))) {
        return HT_ERR_INVALID_ARG;
    }

    status = count_rows(db,
                        "SELECT COUNT(*) FROM analysis_step_results WHERE session_id = ?",
                        session_id,
                        &available);
    if (status != HT_OK) {
        return status;
    }
    *out_count = available;

    status = prepare_statement(db,
                               "SELECT session_id,step_index,pitch_status,timing_status,note_summary "
                               "FROM analysis_step_results WHERE session_id = ? ORDER BY step_index",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, session_id);
    }
    if (status != HT_OK) {
        sqlite3_finalize(statement);
        return status;
    }

    while ((rc = sqlite3_step(statement)) == SQLITE_ROW) {
        if (index < capacity) {
            ht_analysis_step_record* step = &out_steps[index];
            memset(step, 0, sizeof(*step));
            status = copy_text(step->session_id,
                               sizeof(step->session_id),
                               sqlite3_column_text(statement, 0));
            if (status != HT_OK) {
                sqlite3_finalize(statement);
                return status;
            }
            step->step_index = (size_t)sqlite3_column_int64(statement, 1);
            step->pitch_status = (ht_pitch_status)sqlite3_column_int(statement, 2);
            step->timing_status = (ht_timing_status)sqlite3_column_int(statement, 3);
            status = copy_text(step->note_summary,
                               sizeof(step->note_summary),
                               sqlite3_column_text(statement, 4));
            if (status != HT_OK) {
                sqlite3_finalize(statement);
                return status;
            }
        }
        ++index;
    }
    sqlite3_finalize(statement);

    if (rc != SQLITE_DONE) {
        return HT_ERR_DB;
    }
    if (available > capacity) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    return HT_OK;
}

ht_status ht_db_store_advice(ht_db* db, ht_advice_record const* advice) {
    sqlite3_stmt* statement = NULL;
    ht_status status;

    if ((db == NULL) || (advice == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = prepare_statement(db,
                               "INSERT OR REPLACE INTO advice_artifacts "
                               "(advice_id,session_id,created_at,prompt_version,codex_command,"
                               "request_summary,advice_markdown,generation_status) "
                               "VALUES (?,?,?,?,?,?,?,?)",
                               &statement);
    if (status == HT_OK) {
        status = bind_text(statement, 1, advice->advice_id);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 2, advice->session_id);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 3, advice->created_at);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 4, advice->prompt_version);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 5, advice->codex_command);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 6, advice->request_summary);
    }
    if (status == HT_OK) {
        status = bind_text(statement, 7, advice->advice_markdown);
    }
    if (status == HT_OK) {
        sqlite3_bind_int(statement, 8, (int)advice->generation_status);
        status = step_done(statement);
    }

    sqlite3_finalize(statement);
    return status;
}
