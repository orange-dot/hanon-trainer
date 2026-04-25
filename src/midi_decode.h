#ifndef HT_MIDI_DECODE_H
#define HT_MIDI_DECODE_H

#include <stdbool.h>
#include <stddef.h>

#include <hanon_trainer/ht_types.h>

typedef enum ht_midi_decoded_kind {
    HT_MIDI_DECODED_OTHER = 0,
    HT_MIDI_DECODED_NOTE_ON,
    HT_MIDI_DECODED_NOTE_OFF,
    HT_MIDI_DECODED_CONTROLLER,
    HT_MIDI_DECODED_PITCHBEND,
    HT_MIDI_DECODED_PROGRAM_CHANGE,
    HT_MIDI_DECODED_CHANNEL_PRESSURE,
    HT_MIDI_DECODED_KEY_PRESSURE,
    HT_MIDI_DECODED_SYSEX
} ht_midi_decoded_kind;

typedef struct ht_midi_decoded_event {
    ht_midi_decoded_kind kind;
    unsigned channel;
    bool has_channel;
    unsigned note;
    bool has_note;
    unsigned velocity;
    bool has_velocity;
    unsigned controller;
    bool has_controller;
    int value;
    bool has_value;
    unsigned raw_type;
} ht_midi_decoded_event;

ht_status ht_midi_decode_raw_event(unsigned char const* raw_bytes,
                                   size_t raw_byte_count,
                                   ht_midi_decoded_event* out_event);

ht_status ht_midi_decode_sysex_length(size_t sysex_byte_count,
                                      ht_midi_decoded_event* out_event);

char const* ht_midi_decoded_kind_name(ht_midi_decoded_kind kind);

char const* ht_midi_raw_type_name(unsigned raw_type);

#endif
