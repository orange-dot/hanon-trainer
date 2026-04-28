#include <assert.h>

#include <hanon_trainer/guidance_renderer.h>
#include <hanon_trainer/midi_capture.h>
#include <hanon_trainer/score_renderer.h>

int main(void) {
    ht_score_renderer* score_renderer = NULL;
    ht_guidance_renderer* guidance_renderer = NULL;
    ht_midi_capture* capture = NULL;
    ht_midi_device_record devices[16];
    ht_midi_event_record event;
    size_t device_count = 99u;
    ht_status status;
    bool has_event = true;

    assert(ht_score_renderer_create(&score_renderer) == HT_OK);
    assert(ht_score_renderer_probe(score_renderer) == HT_OK);
    ht_score_renderer_destroy(score_renderer);

    assert(ht_guidance_renderer_create(&guidance_renderer) == HT_OK);
    ht_guidance_renderer_destroy(guidance_renderer);

    assert(ht_midi_capture_open(&capture) == HT_OK);
    assert(ht_midi_capture_list_devices(capture, devices, 1u, NULL) == HT_ERR_INVALID_ARG);
    assert(ht_midi_capture_list_devices(capture, NULL, 1u, &device_count) == HT_ERR_INVALID_ARG);
    status = ht_midi_capture_list_devices(capture,
                                          devices,
                                          sizeof(devices) / sizeof(devices[0]),
                                          &device_count);
    assert((status == HT_OK) || (status == HT_ERR_IO) || (status == HT_ERR_BUFFER_TOO_SMALL));
    assert(ht_midi_capture_poll_event(capture, &event, &has_event) == HT_OK);
    assert(!has_event);
    assert(ht_midi_capture_start(capture, "", "session") == HT_ERR_INVALID_ARG);
    assert(ht_midi_capture_start(capture, "fixture-midi", "") == HT_ERR_INVALID_ARG);
    status = ht_midi_capture_start(capture, "fixture-midi", "session");
    assert((status == HT_ERR_NOT_FOUND) || (status == HT_ERR_IO));
    assert(ht_midi_capture_stop(capture) == HT_OK);
    assert(ht_midi_capture_poll_event(capture, &event, &has_event) == HT_OK);
    assert(!has_event);
    ht_midi_capture_close(capture);

    return 0;
}
