#include <hanon_trainer/score_renderer.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include <stdlib.h>

struct ht_score_renderer {
    unsigned linked_sdl_major;
    unsigned linked_ttf_major;
};

ht_status ht_score_renderer_create(ht_score_renderer** out_renderer) {
    ht_score_renderer* renderer;

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

void ht_score_renderer_destroy(ht_score_renderer* renderer) {
    free(renderer);
}

ht_status ht_score_renderer_probe(ht_score_renderer* renderer) {
    SDL_version sdl_version;
    SDL_version const* ttf_version;

    if (renderer == NULL) {
        return HT_ERR_INVALID_ARG;
    }

    SDL_VERSION(&sdl_version);
    ttf_version = TTF_Linked_Version();
    if (ttf_version == NULL) {
        return HT_ERR_UNSUPPORTED;
    }

    renderer->linked_sdl_major = sdl_version.major;
    renderer->linked_ttf_major = ttf_version->major;
    return HT_OK;
}
