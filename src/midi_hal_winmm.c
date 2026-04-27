#include "midi_hal.h"

#include <windows.h>
#include <mmsystem.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HT_WINMM_QUEUE_CAPACITY 256u

typedef struct ht_midi_hal_queue_event {
    ht_midi_hal_event event;
} ht_midi_hal_queue_event;

struct ht_midi_hal {
    HMIDIIN input;
    HANDLE event_available;
    CRITICAL_SECTION lock;
    MIDIHDR sysex_header;
    unsigned char sysex_buffer[HT_MIDI_HAL_RAW_CAPACITY];
    ht_midi_hal_queue_event queue[HT_WINMM_QUEUE_CAPACITY];
    size_t head;
    size_t count;
    LARGE_INTEGER frequency;
    LARGE_INTEGER started;
    bool connected;
    bool lock_initialized;
    bool sysex_prepared;
};

static int64_t elapsed_ns(ht_midi_hal* hal) {
    LARGE_INTEGER now;

    QueryPerformanceCounter(&now);
    return (int64_t)(((now.QuadPart - hal->started.QuadPart) * 1000000000LL)
                     / hal->frequency.QuadPart);
}

static void queue_raw_event(ht_midi_hal* hal, unsigned char const* bytes, size_t byte_count) {
    ht_midi_hal_event event;
    size_t tail;
    size_t copy_count;

    if ((hal == NULL) || (bytes == NULL) || (byte_count == 0u)) {
        return;
    }

    memset(&event, 0, sizeof(event));
    event.event_ns = elapsed_ns(hal);
    event.sysex_byte_count = (bytes[0] == 0xF0u) ? byte_count : 0u;
    copy_count = byte_count;
    if (copy_count > HT_MIDI_HAL_RAW_CAPACITY) {
        copy_count = HT_MIDI_HAL_RAW_CAPACITY;
        event.raw_truncated = true;
    }
    memcpy(event.raw_bytes, bytes, copy_count);
    event.raw_byte_count = copy_count;

    EnterCriticalSection(&hal->lock);
    if (hal->count < HT_WINMM_QUEUE_CAPACITY) {
        tail = (hal->head + hal->count) % HT_WINMM_QUEUE_CAPACITY;
        hal->queue[tail].event = event;
        ++hal->count;
        SetEvent(hal->event_available);
    }
    LeaveCriticalSection(&hal->lock);
}

static size_t short_message_length(unsigned char status) {
    unsigned status_class;

    if (status >= 0xF8u) {
        return 1u;
    }
    status_class = status & 0xF0u;
    if ((status_class == 0xC0u) || (status_class == 0xD0u)) {
        return 2u;
    }
    return 3u;
}

static void CALLBACK midi_input_proc(HMIDIIN input,
                                     UINT message,
                                     DWORD_PTR instance,
                                     DWORD_PTR param1,
                                     DWORD_PTR param2) {
    ht_midi_hal* hal = (ht_midi_hal*)instance;

    (void)param2;
    if (hal == NULL) {
        return;
    }

    if (message == MIM_DATA) {
        DWORD packed = (DWORD)param1;
        unsigned char raw[3];
        size_t raw_count;

        raw[0] = (unsigned char)(packed & 0xFFu);
        raw[1] = (unsigned char)((packed >> 8) & 0xFFu);
        raw[2] = (unsigned char)((packed >> 16) & 0xFFu);
        raw_count = short_message_length(raw[0]);
        queue_raw_event(hal, raw, raw_count);
        return;
    }

    if (message == MIM_LONGDATA) {
        MIDIHDR* header = (MIDIHDR*)param1;
        if ((header != NULL) && (header->dwBytesRecorded > 0u)) {
            queue_raw_event(hal, (unsigned char*)header->lpData, (size_t)header->dwBytesRecorded);
            header->dwBytesRecorded = 0u;
            (void)midiInAddBuffer(input, header, sizeof(*header));
        }
    }
}

static ht_status parse_winmm_index(char const* device_id, UINT* out_index) {
    char* end = NULL;
    unsigned long value;

    if ((device_id == NULL) || (out_index == NULL) || (strncmp(device_id, "winmm:", 6u) != 0)) {
        return HT_ERR_NOT_FOUND;
    }
    value = strtoul(device_id + 6, &end, 10);
    if ((end == device_id + 6) || (*end != '\0') || (value > UINT_MAX)) {
        return HT_ERR_NOT_FOUND;
    }
    *out_index = (UINT)value;
    return HT_OK;
}

char const* ht_midi_hal_backend_name(void) {
    return "winmm";
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
    InitializeCriticalSection(&hal->lock);
    hal->lock_initialized = true;
    hal->event_available = CreateEventA(NULL, TRUE, FALSE, NULL);
    if (hal->event_available == NULL) {
        DeleteCriticalSection(&hal->lock);
        free(hal);
        return HT_ERR_IO;
    }
    QueryPerformanceFrequency(&hal->frequency);
    *out_hal = hal;
    return HT_OK;
}

void ht_midi_hal_close(ht_midi_hal* hal) {
    if (hal == NULL) {
        return;
    }
    (void)ht_midi_hal_disconnect(hal);
    if (hal->event_available != NULL) {
        CloseHandle(hal->event_available);
    }
    if (hal->lock_initialized) {
        DeleteCriticalSection(&hal->lock);
    }
    free(hal);
}

ht_status ht_midi_hal_list_ports(ht_midi_hal* hal,
                                 ht_midi_device_record* out_ports,
                                 size_t capacity,
                                 size_t* out_count) {
    UINT count;
    UINT index;
    size_t written = 0u;

    if (out_count != NULL) {
        *out_count = 0u;
    }
    if ((hal == NULL) || (out_count == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((capacity > 0u) && (out_ports == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    count = midiInGetNumDevs();
    for (index = 0u; index < count; ++index) {
        MIDIINCAPSA caps;
        int printed;

        if (written >= capacity) {
            return HT_ERR_BUFFER_TOO_SMALL;
        }
        if (midiInGetDevCapsA(index, &caps, sizeof(caps)) != MMSYSERR_NOERROR) {
            continue;
        }
        memset(&out_ports[written], 0, sizeof(out_ports[written]));
        printed = snprintf(out_ports[written].device_id,
                           sizeof(out_ports[written].device_id),
                           "winmm:%u",
                           (unsigned)index);
        if ((printed < 0) || ((size_t)printed >= sizeof(out_ports[written].device_id))) {
            return HT_ERR_BUFFER_TOO_SMALL;
        }
        printed = snprintf(out_ports[written].display_name,
                           sizeof(out_ports[written].display_name),
                           "%s",
                           caps.szPname);
        if ((printed < 0) || ((size_t)printed >= sizeof(out_ports[written].display_name))) {
            return HT_ERR_BUFFER_TOO_SMALL;
        }
        out_ports[written].is_default = (written == 0u);
        ++written;
    }

    *out_count = written;
    return HT_OK;
}

ht_status ht_midi_hal_connect(ht_midi_hal* hal, char const* device_id) {
    UINT index;
    MMRESULT result;
    ht_status status;

    if ((hal == NULL) || (device_id == NULL) || (device_id[0] == '\0')) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->connected) {
        return HT_ERR_INVALID_ARG;
    }
    status = parse_winmm_index(device_id, &index);
    if (status != HT_OK) {
        return status;
    }
    if (index >= midiInGetNumDevs()) {
        return HT_ERR_NOT_FOUND;
    }

    result = midiInOpen(&hal->input,
                        index,
                        (DWORD_PTR)midi_input_proc,
                        (DWORD_PTR)hal,
                        CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        hal->input = NULL;
        return HT_ERR_IO;
    }

    memset(&hal->sysex_header, 0, sizeof(hal->sysex_header));
    hal->sysex_header.lpData = (LPSTR)hal->sysex_buffer;
    hal->sysex_header.dwBufferLength = sizeof(hal->sysex_buffer);
    result = midiInPrepareHeader(hal->input, &hal->sysex_header, sizeof(hal->sysex_header));
    if (result == MMSYSERR_NOERROR) {
        hal->sysex_prepared = true;
        (void)midiInAddBuffer(hal->input, &hal->sysex_header, sizeof(hal->sysex_header));
    }

    EnterCriticalSection(&hal->lock);
    hal->head = 0u;
    hal->count = 0u;
    ResetEvent(hal->event_available);
    LeaveCriticalSection(&hal->lock);
    QueryPerformanceCounter(&hal->started);

    result = midiInStart(hal->input);
    if (result != MMSYSERR_NOERROR) {
        (void)ht_midi_hal_disconnect(hal);
        return HT_ERR_IO;
    }

    hal->connected = true;
    return HT_OK;
}

ht_status ht_midi_hal_disconnect(ht_midi_hal* hal) {
    if (hal == NULL) {
        return HT_ERR_INVALID_ARG;
    }
    if (hal->input != NULL) {
        midiInStop(hal->input);
        midiInReset(hal->input);
        if (hal->sysex_prepared) {
            midiInUnprepareHeader(hal->input, &hal->sysex_header, sizeof(hal->sysex_header));
            hal->sysex_prepared = false;
        }
        midiInClose(hal->input);
        hal->input = NULL;
    }
    EnterCriticalSection(&hal->lock);
    hal->head = 0u;
    hal->count = 0u;
    ResetEvent(hal->event_available);
    LeaveCriticalSection(&hal->lock);
    hal->connected = false;
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
    EnterCriticalSection(&hal->lock);
    if (hal->count > 0u) {
        *out_event = hal->queue[hal->head].event;
        hal->head = (hal->head + 1u) % HT_WINMM_QUEUE_CAPACITY;
        --hal->count;
        *out_has_event = true;
    }
    if (hal->count == 0u) {
        ResetEvent(hal->event_available);
    }
    LeaveCriticalSection(&hal->lock);
    return HT_OK;
}

ht_status ht_midi_hal_wait_event(ht_midi_hal* hal, int timeout_ms) {
    DWORD result;

    if ((hal == NULL) || (timeout_ms < 0)) {
        return HT_ERR_INVALID_ARG;
    }
    if (!hal->connected) {
        return HT_OK;
    }
    result = WaitForSingleObject(hal->event_available, (DWORD)timeout_ms);
    if ((result == WAIT_OBJECT_0) || (result == WAIT_TIMEOUT)) {
        return HT_OK;
    }
    return HT_ERR_IO;
}
