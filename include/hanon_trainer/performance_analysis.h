#ifndef HANON_TRAINER_PERFORMANCE_ANALYSIS_H
#define HANON_TRAINER_PERFORMANCE_ANALYSIS_H

#include <hanon_trainer/content_catalog.h>
#include <hanon_trainer/overlay_store.h>
#include <hanon_trainer/sqlite_store.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Analyze a completed session from SQLite events against catalog and overlay data.
 *
 * On success, stores aggregate and step-level results in db and fully
 * initializes out_analysis when non-null.
 */
ht_status ht_analysis_run_session(ht_db* db,
                                  ht_catalog const* catalog,
                                  ht_overlay_store const* overlays,
                                  char const* session_id,
                                  ht_analysis_record* out_analysis);

#ifdef __cplusplus
}
#endif

#endif
