#ifndef HT_MIDI_HAL_H
#define HT_MIDI_HAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <hanon_trainer/ht_types.h>

#define HT_MIDI_HAL_RAW_CAPACITY 1024u

typedef struct ht_midi_hal ht_midi_hal;

typedef struct ht_midi_hal_event {
    int64_t event_ns;
    unsigned char raw_bytes[HT_MIDI_HAL_RAW_CAPACITY];
    size_t raw_byte_count;
    size_t sysex_byte_count;
    bool raw_truncated;
} ht_midi_hal_event;

ht_status ht_midi_hal_open(ht_midi_hal** out_hal);
void ht_midi_hal_close(ht_midi_hal* hal);

ht_status ht_midi_hal_list_ports(ht_midi_hal* hal,
                                 ht_midi_device_record* out_ports,
                                 size_t capacity,
                                 size_t* out_count);

ht_status ht_midi_hal_connect(ht_midi_hal* hal, char const* device_id);
ht_status ht_midi_hal_disconnect(ht_midi_hal* hal);

ht_status ht_midi_hal_poll_event(ht_midi_hal* hal,
                                 ht_midi_hal_event* out_event,
                                 bool* out_has_event);

ht_status ht_midi_hal_wait_event(ht_midi_hal* hal, int timeout_ms);

char const* ht_midi_hal_backend_name(void);

#endif
