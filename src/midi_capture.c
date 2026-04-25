#include <hanon_trainer/midi_capture.h>

#include <alsa/asoundlib.h>

#include <stdlib.h>
#include <string.h>

struct ht_midi_capture {
    char alsa_version[HT_ID_CAPACITY];
    bool active;
};

ht_status ht_midi_capture_open(ht_midi_capture** out_capture) {
    ht_midi_capture* capture;
    char const* version;

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

    version = snd_asoundlib_version();
    if (version != NULL) {
        size_t length = strlen(version);
        if (length >= sizeof(capture->alsa_version)) {
            length = sizeof(capture->alsa_version) - 1u;
        }
        memcpy(capture->alsa_version, version, length);
        capture->alsa_version[length] = '\0';
    }

    *out_capture = capture;
    return HT_OK;
}

void ht_midi_capture_close(ht_midi_capture* capture) {
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

    (void)out_devices;
    (void)capacity;
    return HT_OK;
}

ht_status ht_midi_capture_start(ht_midi_capture* capture,
                                char const* device_id,
                                char const* session_id) {
    if ((capture == NULL) || (device_id == NULL) || (session_id == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    return HT_ERR_UNSUPPORTED;
}

ht_status ht_midi_capture_poll_event(ht_midi_capture* capture,
                                     ht_midi_event_record* out_event,
                                     bool* out_has_event) {
    if (out_has_event != NULL) {
        *out_has_event = false;
    }
    if ((capture == NULL) || (out_event == NULL) || (out_has_event == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    memset(out_event, 0, sizeof(*out_event));
    return HT_OK;
}

ht_status ht_midi_capture_stop(ht_midi_capture* capture) {
    if (capture == NULL) {
        return HT_ERR_INVALID_ARG;
    }
    capture->active = false;
    return HT_OK;
}
