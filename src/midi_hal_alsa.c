#include "midi_hal.h"

#include <alsa/asoundlib.h>

#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct ht_midi_hal {
    snd_seq_t* seq;
    snd_seq_addr_t sender;
    int local_port;
    struct pollfd* poll_descriptors;
    int descriptor_count;
    int64_t started_ns;
    bool connected;
};

static int64_t monotonic_ns(void) {
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    return ((int64_t)now.tv_sec * 1000000000LL) + (int64_t)now.tv_nsec;
}

static ht_status open_sequencer(snd_seq_t** out_seq) {
    snd_seq_t* seq = NULL;
    int rc;

    if (out_seq != NULL) {
        *out_seq = NULL;
    }
    if (out_seq == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    rc = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (rc < 0) {
        return HT_ERR_IO;
    }

    rc = snd_seq_set_client_name(seq, "hanon-trainer MIDI HAL");
    if (rc < 0) {
        snd_seq_close(seq);
        return HT_ERR_IO;
    }

    *out_seq = seq;
    return HT_OK;
}

static unsigned char clamp_7bit(int value) {
    if (value < 0) {
        return 0u;
    }
    if (value > 0x7F) {
        return 0x7Fu;
    }
    return (unsigned char)value;
}

static unsigned char channel_status(unsigned status_class, int alsa_channel) {
    int channel = alsa_channel;

    if (channel < 0) {
        channel = 0;
    }
    if (channel > 15) {
        channel = 15;
    }
    return (unsigned char)(status_class | (unsigned)channel);
}

static unsigned normalize_alsa_pitchbend(int value) {
    long normalized = (long)value + 8192L;

    if (normalized < 0L) {
        return 0u;
    }
    if (normalized > 0x3FFFL) {
        return 0x3FFFu;
    }
    return (unsigned)normalized;
}

static void reset_event(ht_midi_hal* hal, ht_midi_hal_event* out_event) {
    memset(out_event, 0, sizeof(*out_event));
    out_event->event_ns = monotonic_ns() - hal->started_ns;
    if (out_event->event_ns < 0) {
        out_event->event_ns = 0;
    }
}

static ht_status set_raw(ht_midi_hal_event* out_event,
                         unsigned char const* raw,
                         size_t raw_count) {
    if ((out_event == NULL) || (raw == NULL) || (raw_count == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    if (raw_count > HT_MIDI_HAL_RAW_CAPACITY) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(out_event->raw_bytes, raw, raw_count);
    out_event->raw_byte_count = raw_count;
    return HT_OK;
}

static ht_status convert_alsa_event(ht_midi_hal* hal,
                                    snd_seq_event_t const* event,
                                    ht_midi_hal_event* out_event) {
    unsigned char raw[3];
    unsigned pitchbend_value;
    size_t sysex_length;
    size_t copy_count;

    if ((hal == NULL) || (event == NULL) || (out_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    reset_event(hal, out_event);
    switch (event->type) {
    case SND_SEQ_EVENT_NOTEON:
        raw[0] = channel_status(0x90u, event->data.note.channel);
        raw[1] = clamp_7bit(event->data.note.note);
        raw[2] = clamp_7bit(event->data.note.velocity);
        return set_raw(out_event, raw, 3u);
    case SND_SEQ_EVENT_NOTEOFF:
        raw[0] = channel_status(0x80u, event->data.note.channel);
        raw[1] = clamp_7bit(event->data.note.note);
        raw[2] = clamp_7bit(event->data.note.velocity);
        return set_raw(out_event, raw, 3u);
    case SND_SEQ_EVENT_CONTROLLER:
        raw[0] = channel_status(0xB0u, event->data.control.channel);
        raw[1] = clamp_7bit(event->data.control.param);
        raw[2] = clamp_7bit(event->data.control.value);
        return set_raw(out_event, raw, 3u);
    case SND_SEQ_EVENT_PITCHBEND:
        pitchbend_value = normalize_alsa_pitchbend(event->data.control.value);
        raw[0] = channel_status(0xE0u, event->data.control.channel);
        raw[1] = (unsigned char)(pitchbend_value & 0x7Fu);
        raw[2] = (unsigned char)((pitchbend_value >> 7) & 0x7Fu);
        return set_raw(out_event, raw, 3u);
    case SND_SEQ_EVENT_PGMCHANGE:
        raw[0] = channel_status(0xC0u, event->data.control.channel);
        raw[1] = clamp_7bit(event->data.control.value);
        return set_raw(out_event, raw, 2u);
    case SND_SEQ_EVENT_CHANPRESS:
        raw[0] = channel_status(0xD0u, event->data.control.channel);
        raw[1] = clamp_7bit(event->data.control.value);
        return set_raw(out_event, raw, 2u);
    case SND_SEQ_EVENT_KEYPRESS:
        raw[0] = channel_status(0xA0u, event->data.note.channel);
        raw[1] = clamp_7bit(event->data.note.note);
        raw[2] = clamp_7bit(event->data.note.velocity);
        return set_raw(out_event, raw, 3u);
    case SND_SEQ_EVENT_SYSEX:
        sysex_length = (size_t)event->data.ext.len;
        out_event->sysex_byte_count = sysex_length;
        copy_count = sysex_length;
        if (copy_count > HT_MIDI_HAL_RAW_CAPACITY) {
            copy_count = HT_MIDI_HAL_RAW_CAPACITY;
            out_event->raw_truncated = true;
        }
        if ((copy_count > 0u) && (event->data.ext.ptr != NULL)) {
            memcpy(out_event->raw_bytes, event->data.ext.ptr, copy_count);
            out_event->raw_byte_count = copy_count;
        } else {
            out_event->raw_bytes[0] = 0xF0u;
            out_event->raw_byte_count = 1u;
            if (out_event->sysex_byte_count == 0u) {
                out_event->sysex_byte_count = 1u;
            }
        }
        return HT_OK;
    default:
        raw[0] = 0xF8u;
        return set_raw(out_event, raw, 1u);
    }
}

static void close_connection(ht_midi_hal* hal) {
    if (hal == NULL) {
        return;
    }
    if (hal->seq != NULL) {
        if (hal->connected) {
            snd_seq_disconnect_from(hal->seq,
                                    hal->local_port,
                                    hal->sender.client,
                                    hal->sender.port);
        }
        if (hal->local_port >= 0) {
            snd_seq_delete_simple_port(hal->seq, hal->local_port);
        }
        snd_seq_close(hal->seq);
    }
    free(hal->poll_descriptors);
    hal->poll_descriptors = NULL;
    hal->descriptor_count = 0;
    hal->seq = NULL;
    hal->local_port = -1;
    hal->connected = false;
}

char const* ht_midi_hal_backend_name(void) {
    return "alsa";
}

ht_status ht_midi_hal_open(ht_midi_hal** out_hal) {
    ht_midi_hal* hal;

    if (out_hal != NULL) {
        *out_hal = NULL;
    }
    if (out_hal == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    hal = calloc(1u, sizeof(*hal));
    if (hal == NULL) {
        return HT_ERR_IO;
    }
    hal->local_port = -1;
    *out_hal = hal;
    return HT_OK;
}

void ht_midi_hal_close(ht_midi_hal* hal) {
    if (hal == NULL) {
        return;
    }
    close_connection(hal);
    free(hal);
}

ht_status ht_midi_hal_list_ports(ht_midi_hal* hal,
                                 ht_midi_device_record* out_ports,
                                 size_t capacity,
                                 size_t* out_count) {
    snd_seq_t* seq = NULL;
    snd_seq_client_info_t* client_info = NULL;
    snd_seq_port_info_t* port_info = NULL;
    size_t count = 0u;
    ht_status status;
    int rc;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((hal == NULL) || (out_count == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((capacity > 0u) && (out_ports == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = open_sequencer(&seq);
    if (status != HT_OK) {
        return status;
    }
    rc = snd_seq_client_info_malloc(&client_info);
    if (rc < 0) {
        snd_seq_close(seq);
        return HT_ERR_IO;
    }
    rc = snd_seq_port_info_malloc(&port_info);
    if (rc < 0) {
        snd_seq_client_info_free(client_info);
        snd_seq_close(seq);
        return HT_ERR_IO;
    }

    snd_seq_client_info_set_client(client_info, -1);
    while (snd_seq_query_next_client(seq, client_info) >= 0) {
        int client = snd_seq_client_info_get_client(client_info);
        char const* client_name = snd_seq_client_info_get_name(client_info);

        snd_seq_port_info_set_client(port_info, client);
        snd_seq_port_info_set_port(port_info, -1);
        while (snd_seq_query_next_port(seq, port_info) >= 0) {
            unsigned int capability = snd_seq_port_info_get_capability(port_info);

            if (((capability & SND_SEQ_PORT_CAP_READ) != 0u)
                && ((capability & SND_SEQ_PORT_CAP_SUBS_READ) != 0u)) {
                if (count >= capacity) {
                    snd_seq_port_info_free(port_info);
                    snd_seq_client_info_free(client_info);
                    snd_seq_close(seq);
                    return HT_ERR_BUFFER_TOO_SMALL;
                }
                memset(&out_ports[count], 0, sizeof(out_ports[count]));
                rc = snprintf(out_ports[count].device_id,
                              sizeof(out_ports[count].device_id),
                              "alsa:%d:%d",
                              client,
                              snd_seq_port_info_get_port(port_info));
                if ((rc < 0) || ((size_t)rc >= sizeof(out_ports[count].device_id))) {
                    snd_seq_port_info_free(port_info);
                    snd_seq_client_info_free(client_info);
                    snd_seq_close(seq);
                    return HT_ERR_BUFFER_TOO_SMALL;
                }
                rc = snprintf(out_ports[count].display_name,
                              sizeof(out_ports[count].display_name),
                              "%s - %s",
                              client_name,
                              snd_seq_port_info_get_name(port_info));
                if ((rc < 0) || ((size_t)rc >= sizeof(out_ports[count].display_name))) {
                    snd_seq_port_info_free(port_info);
                    snd_seq_client_info_free(client_info);
                    snd_seq_close(seq);
                    return HT_ERR_BUFFER_TOO_SMALL;
                }
                out_ports[count].is_default = (count == 0u);
                ++count;
            }
        }
    }

    *out_count = count;
    snd_seq_port_info_free(port_info);
    snd_seq_client_info_free(client_info);
    snd_seq_close(seq);
    return HT_OK;
}

ht_status ht_midi_hal_connect(ht_midi_hal* hal, char const* device_id) {
    char const* address;
    ht_status status;
    int rc;

    if ((hal == NULL) || (device_id == NULL) || (device_id[0] == '\0')) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->connected) {
        return HT_ERR_INVALID_ARG;
    }
    if (strncmp(device_id, "alsa:", 5u) != 0) {
        return HT_ERR_NOT_FOUND;
    }
    address = device_id + 5;

    status = open_sequencer(&hal->seq);
    if (status != HT_OK) {
        return status;
    }
    rc = snd_seq_parse_address(hal->seq, &hal->sender, address);
    if (rc < 0) {
        close_connection(hal);
        return HT_ERR_NOT_FOUND;
    }

    hal->local_port = snd_seq_create_simple_port(hal->seq,
                                                 "hanon-trainer MIDI input",
                                                 SND_SEQ_PORT_CAP_WRITE
                                                     | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                                 SND_SEQ_PORT_TYPE_APPLICATION);
    if (hal->local_port < 0) {
        close_connection(hal);
        return HT_ERR_IO;
    }

    rc = snd_seq_connect_from(hal->seq, hal->local_port, hal->sender.client, hal->sender.port);
    if (rc < 0) {
        close_connection(hal);
        return HT_ERR_NOT_FOUND;
    }

    hal->descriptor_count = snd_seq_poll_descriptors_count(hal->seq, POLLIN);
    if (hal->descriptor_count <= 0) {
        close_connection(hal);
        return HT_ERR_IO;
    }
    hal->poll_descriptors = calloc((size_t)hal->descriptor_count, sizeof(*hal->poll_descriptors));
    if (hal->poll_descriptors == NULL) {
        close_connection(hal);
        return HT_ERR_IO;
    }
    if (snd_seq_poll_descriptors(hal->seq,
                                 hal->poll_descriptors,
                                 (unsigned)hal->descriptor_count,
                                 POLLIN)
        < 0) {
        close_connection(hal);
        return HT_ERR_IO;
    }

    hal->started_ns = monotonic_ns();
    hal->connected = true;
    return HT_OK;
}

ht_status ht_midi_hal_disconnect(ht_midi_hal* hal) {
    if (hal == NULL) {
        return HT_ERR_INVALID_ARG;
    }
    close_connection(hal);
    return HT_OK;
}

ht_status ht_midi_hal_poll_event(ht_midi_hal* hal,
                                 ht_midi_hal_event* out_event,
                                 bool* out_has_event) {
    snd_seq_event_t* event = NULL;
    int rc;

    if (out_has_event != NULL) {
        *out_has_event = false;
    }
    if ((hal == NULL) || (out_event == NULL) || (out_has_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    memset(out_event, 0, sizeof(*out_event));
    if (!hal->connected || (hal->seq == NULL)) {
        return HT_OK;
    }

    rc = snd_seq_event_input(hal->seq, &event);
    if (rc == -EAGAIN) {
        return HT_OK;
    }
    if (rc < 0) {
        return HT_ERR_IO;
    }
    if (event == NULL) {
        return HT_OK;
    }

    rc = convert_alsa_event(hal, event, out_event);
    snd_seq_free_event(event);
    if (rc != HT_OK) {
        return (ht_status)rc;
    }
    *out_has_event = true;
    return HT_OK;
}

ht_status ht_midi_hal_wait_event(ht_midi_hal* hal, int timeout_ms) {
    int rc;

    if ((hal == NULL) || (timeout_ms < 0)) {
        return HT_ERR_INVALID_ARG;
    }
    if (!hal->connected) {
        return HT_OK;
    }

    rc = poll(hal->poll_descriptors, (nfds_t)hal->descriptor_count, timeout_ms);
    if (rc < 0) {
        if (errno == EINTR) {
            return HT_OK;
        }
        return HT_ERR_IO;
    }
    return HT_OK;
}
