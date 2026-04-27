#include <hanon_trainer/midi_capture.h>

#include "midi_decode.h"
#include "midi_hal.h"

#include <stdlib.h>
#include <string.h>

struct ht_midi_capture {
    ht_midi_hal* hal;
    bool active;
    char session_id[HT_ID_CAPACITY];
    unsigned next_sequence_no;
};

static ht_status validate_borrowed_id(char const* value, size_t capacity) {
    size_t length;

    if ((value == NULL) || (value[0] == '\0') || (capacity == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    length = strlen(value);
    if (length >= capacity) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    return HT_OK;
}

static void copy_raw_bytes(ht_midi_event_record* out_event, ht_midi_hal_event const* event) {
    out_event->status_byte = (event->raw_byte_count > 0u) ? event->raw_bytes[0] : 0u;
    out_event->data_1 = (event->raw_byte_count > 1u) ? event->raw_bytes[1] : 0u;
    out_event->data_2 = (event->raw_byte_count > 2u) ? event->raw_bytes[2] : 0u;
}

static ht_status decode_hal_event(ht_midi_hal_event const* event,
                                  ht_midi_decoded_event* out_decoded) {
    if ((event == NULL) || (out_decoded == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((event->raw_byte_count > 0u) && (event->raw_bytes[0] == 0xF0u)
        && (event->sysex_byte_count > 0u)) {
        return ht_midi_decode_sysex_length(event->sysex_byte_count, out_decoded);
    }
    return ht_midi_decode_raw_event(event->raw_bytes, event->raw_byte_count, out_decoded);
}

static ht_midi_event_kind public_event_kind(ht_midi_decoded_kind kind) {
    switch (kind) {
    case HT_MIDI_DECODED_NOTE_ON:
        return HT_MIDI_EVENT_NOTE_ON;
    case HT_MIDI_DECODED_NOTE_OFF:
        return HT_MIDI_EVENT_NOTE_OFF;
    case HT_MIDI_DECODED_CONTROLLER:
        return HT_MIDI_EVENT_CONTROL_CHANGE;
    case HT_MIDI_DECODED_OTHER:
    case HT_MIDI_DECODED_PITCHBEND:
    case HT_MIDI_DECODED_PROGRAM_CHANGE:
    case HT_MIDI_DECODED_CHANNEL_PRESSURE:
    case HT_MIDI_DECODED_KEY_PRESSURE:
    case HT_MIDI_DECODED_SYSEX:
        return HT_MIDI_EVENT_OTHER;
    }
    return HT_MIDI_EVENT_OTHER;
}

ht_status ht_midi_capture_open(ht_midi_capture** out_capture) {
    ht_midi_capture* capture;
    ht_status status;

    if (out_capture != NULL) {
        *out_capture = NULL;
    }
    if (out_capture == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    capture = calloc(1u, sizeof(*capture));
    if (capture == NULL) {
        return HT_ERR_IO;
    }

    status = ht_midi_hal_open(&capture->hal);
    if (status != HT_OK) {
        free(capture);
        return status;
    }

    *out_capture = capture;
    return HT_OK;
}

void ht_midi_capture_close(ht_midi_capture* capture) {
    if (capture == NULL) {
        return;
    }
    if (capture->active) {
        (void)ht_midi_hal_disconnect(capture->hal);
    }
    ht_midi_hal_close(capture->hal);
    free(capture);
}

ht_status ht_midi_capture_list_devices(ht_midi_capture const* capture,
                                       ht_midi_device_record* out_devices,
                                       size_t capacity,
                                       size_t* out_count) {
    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((capture == NULL) || (out_count == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((capacity > 0u) && (out_devices == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    return ht_midi_hal_list_ports(capture->hal, out_devices, capacity, out_count);
}

ht_status ht_midi_capture_start(ht_midi_capture* capture,
                                char const* device_id,
                                char const* session_id) {
    ht_status status;

    if (capture == NULL) {
        return HT_ERR_INVALID_ARG;
    }
    if (capture->active) {
        return HT_ERR_INVALID_ARG;
    }

    status = validate_borrowed_id(device_id, HT_ID_CAPACITY);
    if (status != HT_OK) {
        return status;
    }
    status = validate_borrowed_id(session_id, sizeof(capture->session_id));
    if (status != HT_OK) {
        return status;
    }

    status = ht_midi_hal_connect(capture->hal, device_id);
    if (status != HT_OK) {
        memset(capture->session_id, 0, sizeof(capture->session_id));
        capture->next_sequence_no = 0u;
        capture->active = false;
        return status;
    }

    memcpy(capture->session_id, session_id, strlen(session_id) + 1u);
    capture->next_sequence_no = 1u;
    capture->active = true;
    return HT_OK;
}

ht_status ht_midi_capture_poll_event(ht_midi_capture* capture,
                                     ht_midi_event_record* out_event,
                                     bool* out_has_event) {
    ht_midi_hal_event hal_event;
    ht_midi_decoded_event decoded;
    ht_status status;
    bool has_event = false;

    if (out_has_event != NULL) {
        *out_has_event = false;
    }
    if ((capture == NULL) || (out_event == NULL) || (out_has_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_event, 0, sizeof(*out_event));
    if (!capture->active) {
        return HT_OK;
    }

    status = ht_midi_hal_poll_event(capture->hal, &hal_event, &has_event);
    if (status != HT_OK) {
        return status;
    }
    if (!has_event) {
        return HT_OK;
    }

    status = decode_hal_event(&hal_event, &decoded);
    if (status != HT_OK) {
        return status;
    }

    memcpy(out_event->session_id, capture->session_id, strlen(capture->session_id) + 1u);
    out_event->sequence_no = capture->next_sequence_no++;
    out_event->event_ns = hal_event.event_ns;
    copy_raw_bytes(out_event, &hal_event);
    out_event->channel = decoded.has_channel ? decoded.channel : 0u;
    out_event->event_kind = public_event_kind(decoded.kind);
    *out_has_event = true;
    return HT_OK;
}

ht_status ht_midi_capture_stop(ht_midi_capture* capture) {
    ht_status status;

    if (capture == NULL) {
        return HT_ERR_INVALID_ARG;
    }
    if (!capture->active) {
        return HT_OK;
    }

    status = ht_midi_hal_disconnect(capture->hal);
    capture->active = false;
    memset(capture->session_id, 0, sizeof(capture->session_id));
    capture->next_sequence_no = 0u;
    return status;
}
