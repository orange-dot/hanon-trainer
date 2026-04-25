#include <hanon_trainer/guidance_renderer.h>

#include <stdlib.h>

struct ht_guidance_renderer {
    unsigned reserved;
};

ht_status ht_guidance_renderer_create(ht_guidance_renderer** out_renderer) {
    ht_guidance_renderer* renderer;

    if (out_renderer != NULL) {
        *out_renderer = NULL;
    }
    if (out_renderer == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    renderer = calloc(1u, sizeof(*renderer));
    if (renderer == NULL) {
        return HT_ERR_IO;
    }

    *out_renderer = renderer;
    return HT_OK;
}

void ht_guidance_renderer_destroy(ht_guidance_renderer* renderer) {
    free(renderer);
}
