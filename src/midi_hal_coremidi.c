#include "midi_hal.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreMIDI/CoreMIDI.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HT_COREMIDI_QUEUE_CAPACITY 256u

typedef struct ht_midi_hal_queue_event {
    ht_midi_hal_event event;
} ht_midi_hal_queue_event;

struct ht_midi_hal {
    MIDIClientRef client;
    MIDIPortRef input_port;
    MIDIEndpointRef source;
    pthread_mutex_t mutex;
    ht_midi_hal_queue_event queue[HT_COREMIDI_QUEUE_CAPACITY];
    size_t head;
    size_t count;
    int64_t started_ns;
    bool connected;
    bool mutex_initialized;
};

static int64_t monotonic_ns(void) {
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }
    return ((int64_t)now.tv_sec * 1000000000LL) + (int64_t)now.tv_nsec;
}

static size_t midi_message_length(unsigned char status, unsigned char const* bytes, size_t length) {
    unsigned status_class;
    size_t index;

    if (status >= 0xF8u) {
        return 1u;
    }
    if (status == 0xF0u) {
        for (index = 1u; index < length; ++index) {
            if (bytes[index] == 0xF7u) {
                return index + 1u;
            }
        }
        return length;
    }
    if (status >= 0xF0u) {
        return 1u;
    }

    status_class = status & 0xF0u;
    if ((status_class == 0xC0u) || (status_class == 0xD0u)) {
        return 2u;
    }
    return 3u;
}

static void queue_raw_event(ht_midi_hal* hal, unsigned char const* bytes, size_t byte_count) {
    ht_midi_hal_event event;
    size_t tail;
    size_t copy_count;

    if ((hal == NULL) || (bytes == NULL) || (byte_count == 0u)) {
        return;
    }

    memset(&event, 0, sizeof(event));
    event.event_ns = monotonic_ns() - hal->started_ns;
    if (event.event_ns < 0) {
        event.event_ns = 0;
    }
    event.sysex_byte_count = (bytes[0] == 0xF0u) ? byte_count : 0u;
    copy_count = byte_count;
    if (copy_count > HT_MIDI_HAL_RAW_CAPACITY) {
        copy_count = HT_MIDI_HAL_RAW_CAPACITY;
        event.raw_truncated = true;
    }
    memcpy(event.raw_bytes, bytes, copy_count);
    event.raw_byte_count = copy_count;

    pthread_mutex_lock(&hal->mutex);
    if (hal->count < HT_COREMIDI_QUEUE_CAPACITY) {
        tail = (hal->head + hal->count) % HT_COREMIDI_QUEUE_CAPACITY;
        hal->queue[tail].event = event;
        ++hal->count;
    }
    pthread_mutex_unlock(&hal->mutex);
}

static void queue_packet_bytes(ht_midi_hal* hal, unsigned char const* bytes, size_t byte_count) {
    size_t offset = 0u;

    while (offset < byte_count) {
        size_t remaining = byte_count - offset;
        size_t message_length;

        if (bytes[offset] < 0x80u) {
            ++offset;
            continue;
        }

        message_length = midi_message_length(bytes[offset], &bytes[offset], remaining);
        if (message_length > remaining) {
            break;
        }
        queue_raw_event(hal, &bytes[offset], message_length);
        offset += message_length;
    }
}

static void midi_read_proc(MIDIPacketList const* packet_list,
                           void* read_proc_ref_con,
                           void* src_conn_ref_con) {
    MIDIPacket const* packet;
    UInt32 index;
    ht_midi_hal* hal = read_proc_ref_con;

    (void)src_conn_ref_con;
    if ((hal == NULL) || (packet_list == NULL)) {
        return;
    }

    packet = &packet_list->packet[0];
    for (index = 0u; index < packet_list->numPackets; ++index) {
        queue_packet_bytes(hal, packet->data, packet->length);
        packet = MIDIPacketNext(packet);
    }
}

static ht_status copy_cf_string(CFStringRef string, char* out, size_t capacity) {
    if ((out == NULL) || (capacity == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    if (string == NULL) {
        return HT_ERR_NOT_FOUND;
    }
    if (!CFStringGetCString(string, out, (CFIndex)capacity, kCFStringEncodingUTF8)) {
        out[0] = '\0';
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    return HT_OK;
}

static ht_status endpoint_device_id(MIDIEndpointRef endpoint,
                                    ItemCount index,
                                    char* out,
                                    size_t capacity) {
    SInt32 unique_id = 0;
    OSStatus status;
    int written;

    status = MIDIObjectGetIntegerProperty(endpoint, kMIDIPropertyUniqueID, &unique_id);
    if (status == noErr) {
        written = snprintf(out, capacity, "coremidi:%d", (int)unique_id);
    } else {
        written = snprintf(out, capacity, "coremidi:index:%lu", (unsigned long)index);
    }
    if ((written < 0) || ((size_t)written >= capacity)) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    return HT_OK;
}

static ht_status endpoint_name(MIDIEndpointRef endpoint, char* out, size_t capacity) {
    CFStringRef name = NULL;
    ht_status status;

    if ((out == NULL) || (capacity == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    if (MIDIObjectGetStringProperty(endpoint, kMIDIPropertyDisplayName, &name) != noErr) {
        if (MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, &name) != noErr) {
            name = NULL;
        }
    }
    if (name == NULL) {
        status = HT_ERR_NOT_FOUND;
    } else {
        status = copy_cf_string(name, out, capacity);
        CFRelease(name);
    }
    if (status == HT_ERR_NOT_FOUND) {
        if (strlen("CoreMIDI Source") >= capacity) {
            return HT_ERR_BUFFER_TOO_SMALL;
        }
        memcpy(out, "CoreMIDI Source", sizeof("CoreMIDI Source"));
        return HT_OK;
    }
    return status;
}

char const* ht_midi_hal_backend_name(void) {
    return "coremidi";
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
    if (pthread_mutex_init(&hal->mutex, NULL) != 0) {
        free(hal);
        return HT_ERR_IO;
    }
    hal->mutex_initialized = true;
    *out_hal = hal;
    return HT_OK;
}

void ht_midi_hal_close(ht_midi_hal* hal) {
    if (hal == NULL) {
        return;
    }
    (void)ht_midi_hal_disconnect(hal);
    if (hal->mutex_initialized) {
        pthread_mutex_destroy(&hal->mutex);
    }
    free(hal);
}

ht_status ht_midi_hal_list_ports(ht_midi_hal* hal,
                                 ht_midi_device_record* out_ports,
                                 size_t capacity,
                                 size_t* out_count) {
    ItemCount source_count;
    ItemCount index;
    size_t written = 0u;
    ht_status status;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((hal == NULL) || (out_count == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((capacity > 0u) && (out_ports == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    source_count = MIDIGetNumberOfSources();
    for (index = 0u; index < source_count; ++index) {
        MIDIEndpointRef endpoint = MIDIGetSource(index);
        if (endpoint == 0) {
            continue;
        }
        if (written >= capacity) {
            return HT_ERR_BUFFER_TOO_SMALL;
        }
        memset(&out_ports[written], 0, sizeof(out_ports[written]));
        status = endpoint_device_id(endpoint,
                                    index,
                                    out_ports[written].device_id,
                                    sizeof(out_ports[written].device_id));
        if (status != HT_OK) {
            return status;
        }
        status = endpoint_name(endpoint,
                               out_ports[written].display_name,
                               sizeof(out_ports[written].display_name));
        if (status != HT_OK) {
            return status;
        }
        out_ports[written].is_default = (written == 0u);
        ++written;
    }

    *out_count = written;
    return HT_OK;
}

ht_status ht_midi_hal_connect(ht_midi_hal* hal, char const* device_id) {
    ItemCount source_count;
    ItemCount index;
    OSStatus os_status;

    if ((hal == NULL) || (device_id == NULL) || (device_id[0] == '\0')) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->connected) {
        return HT_ERR_INVALID_ARG;
    }

    source_count = MIDIGetNumberOfSources();
    for (index = 0u; index < source_count; ++index) {
        char candidate_id[HT_ID_CAPACITY];
        MIDIEndpointRef endpoint = MIDIGetSource(index);

        if (endpoint == 0) {
            continue;
        }
        if (endpoint_device_id(endpoint, index, candidate_id, sizeof(candidate_id)) != HT_OK) {
            continue;
        }
        if (strcmp(candidate_id, device_id) == 0) {
            hal->source = endpoint;
            break;
        }
    }
    if (hal->source == 0) {
        return HT_ERR_NOT_FOUND;
    }

    os_status = MIDIClientCreate(CFSTR("hanon-trainer MIDI HAL"), NULL, NULL, &hal->client);
    if (os_status != noErr) {
        hal->source = 0;
        return HT_ERR_IO;
    }
    os_status =
        MIDIInputPortCreate(hal->client, CFSTR("hanon-trainer MIDI input"), midi_read_proc, hal, &hal->input_port);
    if (os_status != noErr) {
        MIDIClientDispose(hal->client);
        hal->client = 0;
        hal->source = 0;
        return HT_ERR_IO;
    }
    pthread_mutex_lock(&hal->mutex);
    hal->head = 0u;
    hal->count = 0u;
    pthread_mutex_unlock(&hal->mutex);
    hal->started_ns = monotonic_ns();

    os_status = MIDIPortConnectSource(hal->input_port, hal->source, NULL);
    if (os_status != noErr) {
        MIDIPortDispose(hal->input_port);
        MIDIClientDispose(hal->client);
        hal->input_port = 0;
        hal->client = 0;
        hal->source = 0;
        return HT_ERR_IO;
    }

    hal->connected = true;
    return HT_OK;
}

ht_status ht_midi_hal_disconnect(ht_midi_hal* hal) {
    if (hal == NULL) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->input_port != 0) {
        if (hal->source != 0) {
            MIDIPortDisconnectSource(hal->input_port, hal->source);
        }
        MIDIPortDispose(hal->input_port);
    }
    if (hal->client != 0) {
        MIDIClientDispose(hal->client);
    }
    hal->input_port = 0;
    hal->client = 0;
    hal->source = 0;
    hal->connected = false;
    pthread_mutex_lock(&hal->mutex);
    hal->head = 0u;
    hal->count = 0u;
    pthread_mutex_unlock(&hal->mutex);
    return HT_OK;
}

ht_status ht_midi_hal_poll_event(ht_midi_hal* hal,
                                 ht_midi_hal_event* out_event,
                                 bool* out_has_event) {
    if (out_has_event != NULL) {
        *out_has_event = false;
    }
    if ((hal == NULL) || (out_event == NULL) || (out_has_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_event, 0, sizeof(*out_event));
    pthread_mutex_lock(&hal->mutex);
    if (hal->count > 0u) {
        *out_event = hal->queue[hal->head].event;
        hal->head = (hal->head + 1u) % HT_COREMIDI_QUEUE_CAPACITY;
        --hal->count;
        *out_has_event = true;
    }
    pthread_mutex_unlock(&hal->mutex);
    return HT_OK;
}

ht_status ht_midi_hal_wait_event(ht_midi_hal* hal, int timeout_ms) {
    CFTimeInterval seconds;

    if ((hal == NULL) || (timeout_ms < 0)) {
        return HT_ERR_INVALID_ARG;
    }
    if (!hal->connected) {
        return HT_OK;
    }
    seconds = (CFTimeInterval)timeout_ms / 1000.0;
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, seconds, true);
    return HT_OK;
}
