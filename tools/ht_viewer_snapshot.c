#include <hanon_trainer/score_renderer.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum viewer_exit_code {
    VIEWER_EXIT_OK = 0,
    VIEWER_EXIT_INVALID_INPUT = 2,
    VIEWER_EXIT_MISSING_ASSET = 3,
    VIEWER_EXIT_MISSING_OVERLAY = 4,
    VIEWER_EXIT_CORRUPT_ASSET = 5,
    VIEWER_EXIT_OUTPUT_WRITE = 6,
    VIEWER_EXIT_OTHER = 7
};

static void usage(FILE* stream) {
    fprintf(stream,
            "Usage: ht_viewer_snapshot --corpus <path> --asset-root <path> "
            "--variant <id> --step <index> --viewport <WxH> --out <path>\n");
}

static bool set_field(char* destination, size_t capacity, char const* source) {
    int written;

    if ((destination == NULL) || (capacity == 0u) || (source == NULL) || (source[0] == '\0')) {
        return false;
    }
    written = snprintf(destination, capacity, "%s", source);
    return (written >= 0) && ((size_t)written < capacity);
}

static bool parse_size_value(char const* text, size_t* out_value) {
    char* end = NULL;
    unsigned long value;

    if ((text == NULL) || (out_value == NULL) || (text[0] == '\0') || (text[0] == '-')) {
        return false;
    }
    errno = 0;
    value = strtoul(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0')) {
        return false;
    }
    *out_value = (size_t)value;
    return true;
}

static bool parse_viewport(char const* text, unsigned* out_width, unsigned* out_height) {
    char* split;
    char width_text[32];
    char height_text[32];
    unsigned long width;
    unsigned long height;
    char* end = NULL;

    if ((text == NULL) || (out_width == NULL) || (out_height == NULL)) {
        return false;
    }
    split = strchr(text, 'x');
    if (split == NULL) {
        split = strchr(text, 'X');
    }
    if ((split == NULL) || (split == text) || (split[1] == '\0')) {
        return false;
    }
    if (((size_t)(split - text) >= sizeof(width_text)) || (strlen(split + 1) >= sizeof(height_text))) {
        return false;
    }
    memcpy(width_text, text, (size_t)(split - text));
    width_text[split - text] = '\0';
    snprintf(height_text, sizeof(height_text), "%s", split + 1);

    errno = 0;
    width = strtoul(width_text, &end, 10);
    if ((errno != 0) || (end == width_text) || (*end != '\0') || (width == 0u)
        || (width > 4294967295ul)) {
        return false;
    }
    errno = 0;
    height = strtoul(height_text, &end, 10);
    if ((errno != 0) || (end == height_text) || (*end != '\0') || (height == 0u)
        || (height > 4294967295ul)) {
        return false;
    }
    *out_width = (unsigned)width;
    *out_height = (unsigned)height;
    return true;
}

static int map_result_to_exit(ht_status call_status,
                              ht_score_renderer_view_result const* result) {
    if ((call_status == HT_ERR_INVALID_ARG) || (call_status == HT_ERR_BUFFER_TOO_SMALL)) {
        return VIEWER_EXIT_INVALID_INPUT;
    }
    if (result == NULL) {
        return VIEWER_EXIT_OTHER;
    }
    if ((result->asset_status == HT_ERR_NOT_FOUND) || (result->render_status == HT_ERR_NOT_FOUND
                                                       && result->asset_status == HT_ERR_NOT_FOUND)) {
        return VIEWER_EXIT_MISSING_ASSET;
    }
    if ((result->overlay_status == HT_ERR_NOT_FOUND) || (result->overlay_status == HT_ERR_UNSUPPORTED)) {
        return VIEWER_EXIT_MISSING_OVERLAY;
    }
    if ((result->asset_status == HT_ERR_CORRUPT_DATA) || (result->asset_status == HT_ERR_UNSUPPORTED)
        || (result->overlay_status == HT_ERR_CORRUPT_DATA)) {
        return VIEWER_EXIT_CORRUPT_ASSET;
    }
    if ((result->asset_status == HT_OK) && (result->overlay_status == HT_OK)
        && (result->render_status == HT_ERR_IO)) {
        return VIEWER_EXIT_OUTPUT_WRITE;
    }
    if (call_status != HT_OK) {
        return VIEWER_EXIT_OTHER;
    }
    if ((result->catalog_status != HT_OK) || (result->overlay_status != HT_OK)
        || (result->asset_status != HT_OK) || (result->render_status != HT_OK)) {
        return VIEWER_EXIT_OTHER;
    }
    return VIEWER_EXIT_OK;
}

static void print_result(ht_score_renderer_view_result const* result) {
    if (result == NULL) {
        return;
    }
    printf("variant=%s overlay=%s asset=%ux%u viewport=%ux%u "
           "fit=%d,%d,%d,%d active=%d,%d,%d,%d "
           "catalog=%s overlay_status=%s asset_status=%s render=%s path=%s\n",
           result->variant_id,
           result->overlay_id,
           result->source_asset_width_px,
           result->source_asset_height_px,
           result->viewport_width_px,
           result->viewport_height_px,
           result->fitted_score_rect.x,
           result->fitted_score_rect.y,
           result->fitted_score_rect.width,
           result->fitted_score_rect.height,
           result->active_overlay_rect.x,
           result->active_overlay_rect.y,
           result->active_overlay_rect.width,
           result->active_overlay_rect.height,
           ht_status_name(result->catalog_status),
           ht_status_name(result->overlay_status),
           ht_status_name(result->asset_status),
           ht_status_name(result->render_status),
           result->resolved_asset_path);
}

int main(int argc, char** argv) {
    ht_score_renderer_view_request request;
    ht_score_renderer_view_result result;
    ht_score_renderer* renderer = NULL;
    ht_status status;
    int index;
    int exit_code;

    memset(&request, 0, sizeof(request));
    for (index = 1; index < argc; ++index) {
        if ((strcmp(argv[index], "--corpus") == 0) && ((index + 1) < argc)) {
            if (!set_field(request.corpus_root, sizeof(request.corpus_root), argv[++index])) {
                usage(stderr);
                return VIEWER_EXIT_INVALID_INPUT;
            }
        } else if ((strcmp(argv[index], "--asset-root") == 0) && ((index + 1) < argc)) {
            if (!set_field(request.asset_root, sizeof(request.asset_root), argv[++index])) {
                usage(stderr);
                return VIEWER_EXIT_INVALID_INPUT;
            }
        } else if ((strcmp(argv[index], "--variant") == 0) && ((index + 1) < argc)) {
            if (!set_field(request.variant_id, sizeof(request.variant_id), argv[++index])) {
                usage(stderr);
                return VIEWER_EXIT_INVALID_INPUT;
            }
        } else if ((strcmp(argv[index], "--step") == 0) && ((index + 1) < argc)) {
            if (!parse_size_value(argv[++index], &request.step_index)) {
                usage(stderr);
                return VIEWER_EXIT_INVALID_INPUT;
            }
        } else if ((strcmp(argv[index], "--viewport") == 0) && ((index + 1) < argc)) {
            if (!parse_viewport(argv[++index],
                                &request.viewport_width_px,
                                &request.viewport_height_px)) {
                usage(stderr);
                return VIEWER_EXIT_INVALID_INPUT;
            }
        } else if ((strcmp(argv[index], "--out") == 0) && ((index + 1) < argc)) {
            if (!set_field(request.output_path, sizeof(request.output_path), argv[++index])) {
                usage(stderr);
                return VIEWER_EXIT_INVALID_INPUT;
            }
        } else {
            usage(stderr);
            return VIEWER_EXIT_INVALID_INPUT;
        }
    }

    if ((request.corpus_root[0] == '\0') || (request.asset_root[0] == '\0')
        || (request.variant_id[0] == '\0') || (request.viewport_width_px == 0u)
        || (request.viewport_height_px == 0u) || (request.output_path[0] == '\0')) {
        usage(stderr);
        return VIEWER_EXIT_INVALID_INPUT;
    }

    status = ht_score_renderer_create(&renderer);
    if (status != HT_OK) {
        fprintf(stderr, "create renderer failed: %s\n", ht_status_name(status));
        return VIEWER_EXIT_OTHER;
    }
    status = ht_score_renderer_render_view(renderer, &request, &result);
    ht_score_renderer_destroy(renderer);

    print_result(&result);
    exit_code = map_result_to_exit(status, &result);
    if (exit_code != VIEWER_EXIT_OK) {
        fprintf(stderr,
                "viewer snapshot failed: call=%s render=%s asset=%s overlay=%s\n",
                ht_status_name(status),
                ht_status_name(result.render_status),
                ht_status_name(result.asset_status),
                ht_status_name(result.overlay_status));
    }
    return exit_code;
}
