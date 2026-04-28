#include "midi_hal.h"

#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define HT_FAKE_PORT_ID "fake:keyboard"
#define HT_FAKE_PORT_NAME "Fake MIDI Keyboard"

typedef struct ht_fake_event_template {
    int64_t event_ns;
    unsigned char raw_bytes[8];
    size_t raw_byte_count;
    size_t sysex_byte_count;
    bool raw_truncated;
} ht_fake_event_template;

struct ht_midi_hal {
    bool connected;
    size_t next_event;
};

static ht_fake_event_template const ht_fake_events[] = {
    {0, {0x90u, 60u, 64u}, 3u, 0u, false},
    {100000000LL, {0x80u, 60u, 0u}, 3u, 0u, false},
    {200000000LL, {0xB0u, 1u, 127u}, 3u, 0u, false},
    {300000000LL, {0xC0u, 5u}, 2u, 0u, false},
    {400000000LL, {0xA0u, 60u, 45u}, 3u, 0u, false},
    {500000000LL, {0xD0u, 70u}, 2u, 0u, false},
    {600000000LL, {0xE0u, 0u, 64u}, 3u, 0u, false},
    {700000000LL, {0xF0u, 0x7Du, 0x01u, 0xF7u}, 4u, 4u, false},
    {800000000LL, {0xF0u}, 1u, HT_MIDI_HAL_RAW_CAPACITY + 26u, true},
};

static size_t fake_event_count(void) {
    return sizeof ht_fake_events / sizeof ht_fake_events[0];
}

static void sleep_timeout_ms(int timeout_ms) {
    if (timeout_ms <= 0) {
        return;
    }
#ifdef _WIN32
    Sleep((DWORD)timeout_ms);
#else
    {
        struct timespec request;

        request.tv_sec = timeout_ms / 1000;
        request.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
        (void)nanosleep(&request, NULL);
    }
#endif
}

static ht_status copy_text(char* out, size_t capacity, char const* value) {
    size_t length;

    if ((out == NULL) || (capacity == 0u) || (value == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    length = strlen(value);
    if (length >= capacity) {
        out[0] = '\0';
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(out, value, length + 1u);
    return HT_OK;
}

char const* ht_midi_hal_backend_name(void) {
    return "fake";
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

    *out_hal = hal;
    return HT_OK;
}

void ht_midi_hal_close(ht_midi_hal* hal) {
    free(hal);
}

ht_status ht_midi_hal_list_ports(ht_midi_hal* hal,
                                 ht_midi_device_record* out_ports,
                                 size_t capacity,
                                 size_t* out_count) {
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
    if (capacity == 0u) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }

    memset(&out_ports[0], 0, sizeof(out_ports[0]));
    status = copy_text(out_ports[0].device_id, sizeof(out_ports[0].device_id), HT_FAKE_PORT_ID);
    if (status != HT_OK) {
        return status;
    }
    status = copy_text(out_ports[0].display_name,
                       sizeof(out_ports[0].display_name),
                       HT_FAKE_PORT_NAME);
    if (status != HT_OK) {
        return status;
    }
    out_ports[0].is_default = true;
    *out_count = 1u;
    return HT_OK;
}

ht_status ht_midi_hal_connect(ht_midi_hal* hal, char const* device_id) {
    if ((hal == NULL) || (device_id == NULL) || (device_id[0] == '\0')) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->connected) {
        return HT_ERR_INVALID_ARG;
    }
    if (strcmp(device_id, HT_FAKE_PORT_ID) != 0) {
        return HT_ERR_NOT_FOUND;
    }

    hal->connected = true;
    hal->next_event = 0u;
    return HT_OK;
}

ht_status ht_midi_hal_disconnect(ht_midi_hal* hal) {
    if (hal == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    hal->connected = false;
    hal->next_event = 0u;
    return HT_OK;
}

ht_status ht_midi_hal_poll_event(ht_midi_hal* hal,
                                 ht_midi_hal_event* out_event,
                                 bool* out_has_event) {
    ht_fake_event_template const* source;

    if (out_has_event != NULL) {
        *out_has_event = false;
    }
    if ((hal == NULL) || (out_event == NULL) || (out_has_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_event, 0, sizeof(*out_event));
    if (!hal->connected || (hal->next_event >= fake_event_count())) {
        return HT_OK;
    }

    source = &ht_fake_events[hal->next_event];
    out_event->event_ns = source->event_ns;
    out_event->raw_byte_count = source->raw_byte_count;
    out_event->sysex_byte_count = source->sysex_byte_count;
    out_event->raw_truncated = source->raw_truncated;
    memcpy(out_event->raw_bytes, source->raw_bytes, source->raw_byte_count);
    if (source->raw_truncated && (source->raw_byte_count < HT_MIDI_HAL_RAW_CAPACITY)) {
        memset(&out_event->raw_bytes[source->raw_byte_count],
               0x7Du,
               HT_MIDI_HAL_RAW_CAPACITY - source->raw_byte_count);
        out_event->raw_byte_count = HT_MIDI_HAL_RAW_CAPACITY;
    }

    ++hal->next_event;
    *out_has_event = true;
    return HT_OK;
}

ht_status ht_midi_hal_wait_event(ht_midi_hal* hal, int timeout_ms) {
    if ((hal == NULL) || (timeout_ms < 0)) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->connected && (hal->next_event < fake_event_count())) {
        return HT_OK;
    }
    sleep_timeout_ms(timeout_ms);
    return HT_OK;
}
