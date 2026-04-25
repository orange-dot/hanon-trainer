#include <assert.h>

#include "midi_decode.h"

static void expect_note_on(void) {
    unsigned char raw[] = {0x91u, 64u, 96u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(raw, sizeof(raw), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_NOTE_ON);
    assert(event.has_channel);
    assert(event.channel == 2u);
    assert(event.has_note);
    assert(event.note == 64u);
    assert(event.has_velocity);
    assert(event.velocity == 96u);
    assert(event.raw_type == 0x90u);
    assert(ht_midi_decoded_kind_name(event.kind) != NULL);
    assert(ht_midi_raw_type_name(event.raw_type) != NULL);
}

static void expect_note_on_velocity_zero_as_note_off(void) {
    unsigned char raw[] = {0x90u, 60u, 0u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(raw, sizeof(raw), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_NOTE_OFF);
    assert(event.has_velocity);
    assert(event.velocity == 0u);
    assert(event.raw_type == 0x90u);
}

static void expect_note_off_release_velocity(void) {
    unsigned char raw[] = {0x80u, 60u, 23u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(raw, sizeof(raw), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_NOTE_OFF);
    assert(event.has_note);
    assert(event.note == 60u);
    assert(event.has_velocity);
    assert(event.velocity == 23u);
}

static void expect_controller(void) {
    unsigned char raw[] = {0xB0u, 64u, 127u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(raw, sizeof(raw), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_CONTROLLER);
    assert(event.has_controller);
    assert(event.controller == 64u);
    assert(event.has_value);
    assert(event.value == 127);
}

static void expect_pitchbend(void) {
    unsigned char raw[] = {0xE0u, 0x00u, 0x40u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(raw, sizeof(raw), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_PITCHBEND);
    assert(event.has_value);
    assert(event.value == 8192);
}

static void expect_program_and_pressure(void) {
    unsigned char program[] = {0xC0u, 106u};
    unsigned char channel_pressure[] = {0xD0u, 127u};
    unsigned char key_pressure[] = {0xA0u, 65u, 80u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(program, sizeof(program), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_PROGRAM_CHANGE);
    assert(event.has_value);
    assert(event.value == 106);

    assert(ht_midi_decode_raw_event(channel_pressure, sizeof(channel_pressure), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_CHANNEL_PRESSURE);
    assert(event.has_value);
    assert(event.value == 127);

    assert(ht_midi_decode_raw_event(key_pressure, sizeof(key_pressure), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_KEY_PRESSURE);
    assert(event.has_note);
    assert(event.note == 65u);
    assert(!event.has_velocity);
    assert(event.has_value);
    assert(event.value == 80);
}

static void expect_sysex_length(void) {
    unsigned char raw[] = {0xF0u, 0x7Eu, 0x7Fu, 0x06u, 0x02u, 0xF7u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(raw, sizeof(raw), &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_SYSEX);
    assert(!event.has_channel);
    assert(event.has_value);
    assert(event.value == 6);

    assert(ht_midi_decode_sysex_length(42u, &event) == HT_OK);
    assert(event.kind == HT_MIDI_DECODED_SYSEX);
    assert(event.value == 42);
}

static void expect_failures(void) {
    unsigned char truncated_note[] = {0x90u, 60u};
    unsigned char bad_data_byte[] = {0x90u, 60u, 0x80u};
    unsigned char no_status_byte[] = {0x40u, 60u, 1u};
    ht_midi_decoded_event event;

    assert(ht_midi_decode_raw_event(NULL, 3u, &event) == HT_ERR_INVALID_ARG);
    assert(ht_midi_decode_raw_event(truncated_note, 0u, &event) == HT_ERR_INVALID_ARG);
    assert(ht_midi_decode_raw_event(truncated_note, sizeof(truncated_note), NULL)
           == HT_ERR_INVALID_ARG);
    assert(ht_midi_decode_raw_event(truncated_note, sizeof(truncated_note), &event)
           == HT_ERR_CORRUPT_DATA);
    assert(ht_midi_decode_raw_event(bad_data_byte, sizeof(bad_data_byte), &event)
           == HT_ERR_CORRUPT_DATA);
    assert(ht_midi_decode_raw_event(no_status_byte, sizeof(no_status_byte), &event)
           == HT_ERR_CORRUPT_DATA);
    assert(ht_midi_decode_sysex_length(0u, &event) == HT_ERR_INVALID_ARG);
}

int main(void) {
    expect_note_on();
    expect_note_on_velocity_zero_as_note_off();
    expect_note_off_release_velocity();
    expect_controller();
    expect_pitchbend();
    expect_program_and_pressure();
    expect_sysex_length();
    expect_failures();

    return 0;
}
