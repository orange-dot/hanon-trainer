#include "midi_decode.h"

#include <limits.h>
#include <string.h>

static bool all_data_bytes(unsigned char const* raw_bytes, size_t raw_byte_count) {
    size_t index;

    for (index = 1u; index < raw_byte_count; ++index) {
        if (raw_bytes[index] > 0x7Fu) {
            return false;
        }
    }
    return true;
}

static ht_status require_message_shape(unsigned char const* raw_bytes,
                                       size_t raw_byte_count,
                                       size_t expected_count) {
    if ((raw_bytes == NULL) || (raw_byte_count != expected_count)) {
        return HT_ERR_CORRUPT_DATA;
    }
    if (!all_data_bytes(raw_bytes, raw_byte_count)) {
        return HT_ERR_CORRUPT_DATA;
    }
    return HT_OK;
}

static void reset_event(ht_midi_decoded_event* out_event) {
    memset(out_event, 0, sizeof(*out_event));
    out_event->kind = HT_MIDI_DECODED_OTHER;
}

static void set_channel(ht_midi_decoded_event* out_event, unsigned char status_byte) {
    out_event->channel = (unsigned)(status_byte & 0x0Fu) + 1u;
    out_event->has_channel = true;
}

ht_status ht_midi_decode_sysex_length(size_t sysex_byte_count,
                                      ht_midi_decoded_event* out_event) {
    if ((out_event == NULL) || (sysex_byte_count == 0u) || (sysex_byte_count > (size_t)INT_MAX)) {
        return HT_ERR_INVALID_ARG;
    }

    reset_event(out_event);
    out_event->kind = HT_MIDI_DECODED_SYSEX;
    out_event->value = (int)sysex_byte_count;
    out_event->has_value = true;
    out_event->raw_type = 0xF0u;
    return HT_OK;
}

ht_status ht_midi_decode_raw_event(unsigned char const* raw_bytes,
                                   size_t raw_byte_count,
                                   ht_midi_decoded_event* out_event) {
    unsigned char status_byte;
    unsigned status_class;
    ht_status status;

    if ((raw_bytes == NULL) || (raw_byte_count == 0u) || (out_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    reset_event(out_event);
    status_byte = raw_bytes[0];
    if (status_byte < 0x80u) {
        return HT_ERR_CORRUPT_DATA;
    }

    if ((status_byte >= 0xF0u) && (status_byte <= 0xF7u)) {
        return ht_midi_decode_sysex_length(raw_byte_count, out_event);
    }

    if (status_byte >= 0xF8u) {
        out_event->raw_type = (unsigned)status_byte;
        return HT_OK;
    }

    status_class = (unsigned)(status_byte & 0xF0u);
    out_event->raw_type = status_class;
    set_channel(out_event, status_byte);

    switch (status_class) {
    case 0x80u:
        status = require_message_shape(raw_bytes, raw_byte_count, 3u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind = HT_MIDI_DECODED_NOTE_OFF;
        out_event->note = raw_bytes[1];
        out_event->has_note = true;
        out_event->velocity = raw_bytes[2];
        out_event->has_velocity = true;
        return HT_OK;
    case 0x90u:
        status = require_message_shape(raw_bytes, raw_byte_count, 3u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind =
            (raw_bytes[2] == 0u) ? HT_MIDI_DECODED_NOTE_OFF : HT_MIDI_DECODED_NOTE_ON;
        out_event->note = raw_bytes[1];
        out_event->has_note = true;
        out_event->velocity = raw_bytes[2];
        out_event->has_velocity = true;
        return HT_OK;
    case 0xA0u:
        status = require_message_shape(raw_bytes, raw_byte_count, 3u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind = HT_MIDI_DECODED_KEY_PRESSURE;
        out_event->note = raw_bytes[1];
        out_event->has_note = true;
        out_event->value = raw_bytes[2];
        out_event->has_value = true;
        return HT_OK;
    case 0xB0u:
        status = require_message_shape(raw_bytes, raw_byte_count, 3u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind = HT_MIDI_DECODED_CONTROLLER;
        out_event->controller = raw_bytes[1];
        out_event->has_controller = true;
        out_event->value = raw_bytes[2];
        out_event->has_value = true;
        return HT_OK;
    case 0xC0u:
        status = require_message_shape(raw_bytes, raw_byte_count, 2u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind = HT_MIDI_DECODED_PROGRAM_CHANGE;
        out_event->value = raw_bytes[1];
        out_event->has_value = true;
        return HT_OK;
    case 0xD0u:
        status = require_message_shape(raw_bytes, raw_byte_count, 2u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind = HT_MIDI_DECODED_CHANNEL_PRESSURE;
        out_event->value = raw_bytes[1];
        out_event->has_value = true;
        return HT_OK;
    case 0xE0u:
        status = require_message_shape(raw_bytes, raw_byte_count, 3u);
        if (status != HT_OK) {
            return status;
        }
        out_event->kind = HT_MIDI_DECODED_PITCHBEND;
        out_event->value = (int)raw_bytes[1] | ((int)raw_bytes[2] << 7);
        out_event->has_value = true;
        return HT_OK;
    default:
        out_event->kind = HT_MIDI_DECODED_OTHER;
        return HT_OK;
    }
}

char const* ht_midi_decoded_kind_name(ht_midi_decoded_kind kind) {
    switch (kind) {
    case HT_MIDI_DECODED_NOTE_ON:
        return "note_on";
    case HT_MIDI_DECODED_NOTE_OFF:
        return "note_off";
    case HT_MIDI_DECODED_CONTROLLER:
        return "controller";
    case HT_MIDI_DECODED_PITCHBEND:
        return "pitchbend";
    case HT_MIDI_DECODED_PROGRAM_CHANGE:
        return "pgmchange";
    case HT_MIDI_DECODED_CHANNEL_PRESSURE:
        return "chanpress";
    case HT_MIDI_DECODED_KEY_PRESSURE:
        return "keypress";
    case HT_MIDI_DECODED_SYSEX:
        return "sysex";
    case HT_MIDI_DECODED_OTHER:
    default:
        return "other";
    }
}

char const* ht_midi_raw_type_name(unsigned raw_type) {
    switch (raw_type) {
    case 0x80u:
        return "NOTEOFF";
    case 0x90u:
        return "NOTEON";
    case 0xA0u:
        return "KEYPRESS";
    case 0xB0u:
        return "CONTROLLER";
    case 0xC0u:
        return "PGMCHANGE";
    case 0xD0u:
        return "CHANPRESS";
    case 0xE0u:
        return "PITCHBEND";
    case 0xF0u:
        return "SYSEX";
    default:
        return "OTHER";
    }
}
