#ifndef HANON_TRAINER_SESSION_CORE_H
#define HANON_TRAINER_SESSION_CORE_H

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create an in-memory session handle.
 *
 * On success, out_session receives ownership of a non-null handle.
 */
ht_status ht_session_create(ht_session** out_session);

/**
 * Release a session handle.
 *
 * Passing NULL is allowed.
 */
void ht_session_destroy(ht_session* session);

/**
 * Set the selected variant from a borrowed NUL-terminated variant_id.
 */
ht_status ht_session_select_variant(ht_session* session, char const* variant_id);

/**
 * Set the target tempo in beats per minute.
 */
ht_status ht_session_set_target_tempo(ht_session* session, unsigned target_tempo);

/**
 * Set the preferred MIDI device id from a borrowed NUL-terminated string.
 */
ht_status ht_session_arm_device(ht_session* session, char const* midi_device_id);

#ifdef __cplusplus
}
#endif

#endif
