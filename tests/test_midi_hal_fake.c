#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include <hanon_trainer/midi_capture.h>

#include "midi_hal.h"

static void expect_fake_port_list(void) {
    ht_midi_hal* hal = NULL;
    ht_midi_device_record ports[1];
    size_t port_count = 0u;

    assert(ht_midi_hal_open(&hal) == HT_OK);
    assert(hal != NULL);
    assert(strcmp(ht_midi_hal_backend_name(), "fake") == 0);

    assert(ht_midi_hal_list_ports(hal, ports, 0u, &port_count) == HT_ERR_BUFFER_TOO_SMALL);
    assert(port_count == 0u);
    assert(ht_midi_hal_list_ports(hal, ports, 1u, &port_count) == HT_OK);
    assert(port_count == 1u);
    assert(strcmp(ports[0].device_id, "fake:keyboard") == 0);
    assert(strcmp(ports[0].display_name, "Fake MIDI Keyboard") == 0);
    assert(ports[0].is_default);

    ht_midi_hal_close(hal);
}

static void expect_fake_event_stream(void) {
    ht_midi_hal* hal = NULL;
    ht_midi_hal_event event;
    ht_midi_hal_event last_event;
    bool has_event = false;
    size_t event_count = 0u;

    assert(ht_midi_hal_open(&hal) == HT_OK);
    assert(ht_midi_hal_wait_event(hal, -1) == HT_ERR_INVALID_ARG);
    assert(ht_midi_hal_connect(hal, "missing") == HT_ERR_NOT_FOUND);
    assert(ht_midi_hal_connect(hal, "fake:keyboard") == HT_OK);

    assert(ht_midi_hal_wait_event(hal, 0) == HT_OK);
    assert(ht_midi_hal_poll_event(hal, &event, &has_event) == HT_OK);
    assert(has_event);
    assert(event.event_ns == 0);
    assert(event.raw_byte_count == 3u);
    assert(event.raw_bytes[0] == 0x90u);
    assert(event.raw_bytes[1] == 60u);
    assert(event.raw_bytes[2] == 64u);
    ++event_count;
    last_event = event;

    while (true) {
        assert(ht_midi_hal_poll_event(hal, &event, &has_event) == HT_OK);
        if (!has_event) {
            break;
        }
        ++event_count;
        last_event = event;
    }
    assert(event_count == 9u);
    assert(last_event.raw_byte_count == HT_MIDI_HAL_RAW_CAPACITY);
    assert(last_event.sysex_byte_count == HT_MIDI_HAL_RAW_CAPACITY + 26u);
    assert(last_event.raw_truncated);
    assert(last_event.raw_bytes[0] == 0xF0u);

    assert(ht_midi_hal_disconnect(hal) == HT_OK);
    ht_midi_hal_close(hal);
}

static void expect_capture_mapping(void) {
    ht_midi_capture* capture = NULL;
    ht_midi_device_record devices[1];
    ht_midi_event_record event;
    size_t device_count = 0u;
    bool has_event = false;

    assert(ht_midi_capture_open(&capture) == HT_OK);
    assert(ht_midi_capture_list_devices(capture, devices, 1u, &device_count) == HT_OK);
    assert(device_count == 1u);
    assert(strcmp(devices[0].device_id, "fake:keyboard") == 0);

    assert(ht_midi_capture_start(capture, "fake:keyboard", "session-1") == HT_OK);
    assert(ht_midi_capture_poll_event(capture, &event, &has_event) == HT_OK);
    assert(has_event);
    assert(strcmp(event.session_id, "session-1") == 0);
    assert(event.sequence_no == 1u);
    assert(event.event_kind == HT_MIDI_EVENT_NOTE_ON);
    assert(event.status_byte == 0x90u);
    assert(event.data_1 == 60u);
    assert(event.data_2 == 64u);
    assert(event.channel == 1u);

    assert(ht_midi_capture_poll_event(capture, &event, &has_event) == HT_OK);
    assert(has_event);
    assert(event.sequence_no == 2u);
    assert(event.event_kind == HT_MIDI_EVENT_NOTE_OFF);

    assert(ht_midi_capture_poll_event(capture, &event, &has_event) == HT_OK);
    assert(has_event);
    assert(event.sequence_no == 3u);
    assert(event.event_kind == HT_MIDI_EVENT_CONTROL_CHANGE);
    assert(event.status_byte == 0xB0u);
    assert(event.data_1 == 1u);
    assert(event.data_2 == 127u);

    assert(ht_midi_capture_stop(capture) == HT_OK);
    assert(ht_midi_capture_poll_event(capture, &event, &has_event) == HT_OK);
    assert(!has_event);
    ht_midi_capture_close(capture);
}

int main(void) {
    expect_fake_port_list();
    expect_fake_event_stream();
    expect_capture_mapping();
    return 0;
}
