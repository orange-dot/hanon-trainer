#ifndef HANON_TRAINER_MIDI_CAPTURE_H
#define HANON_TRAINER_MIDI_CAPTURE_H

#include <stdbool.h>
#include <stddef.h>

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open the local MIDI capture boundary.
 *
 * On success, out_capture receives ownership of a non-null capture handle.
 */
ht_status ht_midi_capture_open(ht_midi_capture** out_capture);

/**
 * Release a capture handle.
 *
 * Passing NULL is allowed.
 */
void ht_midi_capture_close(ht_midi_capture* capture);

/**
 * List available MIDI devices into caller-owned storage.
 *
 * capacity is an element count. This first slice does not enumerate live ALSA
 * devices and returns HT_OK with out_count set to zero.
 */
ht_status ht_midi_capture_list_devices(ht_midi_capture const* capture,
                                       ht_midi_device_record* out_devices,
                                       size_t capacity,
                                       size_t* out_count);

/**
 * Start capture for a borrowed device_id and session_id.
 *
 * Live capture is intentionally unsupported in this slice.
 */
ht_status ht_midi_capture_start(ht_midi_capture* capture,
                                char const* device_id,
                                char const* session_id);

/**
 * Poll one captured event.
 *
 * An empty poll returns HT_OK with out_has_event set false and does not write a
 * meaningful event record.
 */
ht_status ht_midi_capture_poll_event(ht_midi_capture* capture,
                                     ht_midi_event_record* out_event,
                                     bool* out_has_event);

/**
 * Stop capture if active.
 */
ht_status ht_midi_capture_stop(ht_midi_capture* capture);

#ifdef __cplusplus
}
#endif

#endif
