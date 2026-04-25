#include <hanon_trainer/session_core.h>

#include <stdlib.h>
#include <string.h>

struct ht_session {
    char variant_id[HT_ID_CAPACITY];
    unsigned target_tempo;
    char midi_device_id[HT_ID_CAPACITY];
};

static ht_status copy_text(char* destination, size_t capacity, char const* source) {
    size_t length;

    if ((destination == NULL) || (capacity == 0u) || (source == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    length = strlen(source);
    if (length >= capacity) {
        destination[0] = '\0';
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(destination, source, length + 1u);
    return HT_OK;
}

ht_status ht_session_create(ht_session** out_session) {
    ht_session* session;

    if (out_session != NULL) {
        *out_session = NULL;
    }
    if (out_session == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    session = calloc(1u, sizeof(*session));
    if (session == NULL) {
        return HT_ERR_IO;
    }

    *out_session = session;
    return HT_OK;
}

void ht_session_destroy(ht_session* session) {
    free(session);
}

ht_status ht_session_select_variant(ht_session* session, char const* variant_id) {
    if ((session == NULL) || (variant_id == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    return copy_text(session->variant_id, sizeof(session->variant_id), variant_id);
}

ht_status ht_session_set_target_tempo(ht_session* session, unsigned target_tempo) {
    if ((session == NULL) || (target_tempo == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    session->target_tempo = target_tempo;
    return HT_OK;
}

ht_status ht_session_arm_device(ht_session* session, char const* midi_device_id) {
    if ((session == NULL) || (midi_device_id == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    return copy_text(session->midi_device_id, sizeof(session->midi_device_id), midi_device_id);
}
