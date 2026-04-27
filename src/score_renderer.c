#include <hanon_trainer/score_renderer.h>

#include <hanon_trainer/content_catalog.h>
#include <hanon_trainer/overlay_store.h>

#include <SDL.h>
#include <SDL_ttf.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ppm_image {
    unsigned width;
    unsigned height;
    unsigned char* rgb;
    size_t rgb_size;
} ppm_image;

struct ht_score_renderer {
    unsigned linked_sdl_major;
    unsigned linked_ttf_major;
};

static size_t bounded_text_length(char const* text, size_t capacity, bool* out_valid) {
    size_t length;

    if (out_valid != NULL) {
        *out_valid = false;
    }
    if ((text == NULL) || (capacity == 0u)) {
        return 0u;
    }
    for (length = 0u; length < capacity; ++length) {
        if (text[length] == '\0') {
            if (out_valid != NULL) {
                *out_valid = true;
            }
            return length;
        }
    }
    return capacity;
}

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

static ht_status copy_request_text(char* destination,
                                   size_t destination_capacity,
                                   char const* source,
                                   size_t source_capacity) {
    bool valid = false;
    size_t length = bounded_text_length(source, source_capacity, &valid);

    if ((destination == NULL) || (destination_capacity == 0u) || !valid) {
        return HT_ERR_INVALID_ARG;
    }
    if (length >= destination_capacity) {
        destination[0] = '\0';
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(destination, source, length + 1u);
    return HT_OK;
}

static bool request_text_present(char const* text, size_t capacity) {
    bool valid = false;
    size_t length = bounded_text_length(text, capacity, &valid);

    return valid && (length > 0u);
}

static bool path_is_absolute(char const* path) {
    if ((path == NULL) || (path[0] == '\0')) {
        return false;
    }
    if ((path[0] == '/') || (path[0] == '\\')) {
        return true;
    }
    return (((path[0] >= 'A') && (path[0] <= 'Z')) || ((path[0] >= 'a') && (path[0] <= 'z')))
           && (path[1] == ':');
}

static ht_status join_path(char* destination,
                           size_t capacity,
                           char const* root,
                           char const* relative_path) {
    int written;

    if ((destination == NULL) || (root == NULL) || (relative_path == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if (path_is_absolute(relative_path)) {
        return copy_text(destination, capacity, relative_path);
    }
    written = snprintf(destination, capacity, "%s/%s", root, relative_path);
    if ((written < 0) || ((size_t)written >= capacity)) {
        if (capacity > 0u) {
            destination[0] = '\0';
        }
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    return HT_OK;
}

static bool ppm_is_space(unsigned char value) {
    return (value == ' ') || (value == '\t') || (value == '\n') || (value == '\r')
           || (value == '\f') || (value == '\v');
}

static void ppm_skip_ws_and_comments(unsigned char const* data, size_t size, size_t* position) {
    bool skipped;

    if ((data == NULL) || (position == NULL)) {
        return;
    }
    do {
        skipped = false;
        while ((*position < size) && ppm_is_space(data[*position])) {
            ++(*position);
            skipped = true;
        }
        if ((*position < size) && (data[*position] == '#')) {
            while ((*position < size) && (data[*position] != '\n') && (data[*position] != '\r')) {
                ++(*position);
            }
            skipped = true;
        }
    } while (skipped);
}

static ht_status ppm_next_token(unsigned char const* data,
                                size_t size,
                                size_t* position,
                                char* token,
                                size_t token_capacity) {
    size_t start;
    size_t length;

    if ((data == NULL) || (position == NULL) || (token == NULL) || (token_capacity == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    ppm_skip_ws_and_comments(data, size, position);
    if (*position >= size) {
        return HT_ERR_CORRUPT_DATA;
    }
    start = *position;
    while ((*position < size) && !ppm_is_space(data[*position]) && (data[*position] != '#')) {
        ++(*position);
    }
    length = *position - start;
    if ((length == 0u) || (length >= token_capacity)) {
        token[0] = '\0';
        return HT_ERR_CORRUPT_DATA;
    }
    memcpy(token, &data[start], length);
    token[length] = '\0';
    return HT_OK;
}

static ht_status parse_unsigned_token(char const* token, unsigned* out_value) {
    char* end = NULL;
    unsigned long value;

    if ((token == NULL) || (out_value == NULL) || (token[0] == '\0') || (token[0] == '-')) {
        return HT_ERR_CORRUPT_DATA;
    }
    errno = 0;
    value = strtoul(token, &end, 10);
    if ((errno != 0) || (end == token) || (*end != '\0') || (value > UINT_MAX)) {
        return HT_ERR_CORRUPT_DATA;
    }
    *out_value = (unsigned)value;
    return HT_OK;
}

static ht_status read_file(char const* path, unsigned char** out_data, size_t* out_size) {
    FILE* file;
    long file_size;
    unsigned char* data;
    size_t bytes_read;

    if (out_data != NULL) {
        *out_data = NULL;
    }
    if (out_size != NULL) {
        *out_size = 0u;
    }
    if ((path == NULL) || (out_data == NULL) || (out_size == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    file = fopen(path, "rb");
    if (file == NULL) {
        return (errno == ENOENT) ? HT_ERR_NOT_FOUND : HT_ERR_IO;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return HT_ERR_IO;
    }
    file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return HT_ERR_IO;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return HT_ERR_IO;
    }
    if (file_size == 0) {
        fclose(file);
        return HT_ERR_CORRUPT_DATA;
    }

    data = malloc((size_t)file_size);
    if (data == NULL) {
        fclose(file);
        return HT_ERR_IO;
    }
    bytes_read = fread(data, 1u, (size_t)file_size, file);
    if (fclose(file) != 0) {
        free(data);
        return HT_ERR_IO;
    }
    if (bytes_read != (size_t)file_size) {
        free(data);
        return HT_ERR_IO;
    }
    *out_data = data;
    *out_size = (size_t)file_size;
    return HT_OK;
}

static void ppm_image_destroy(ppm_image* image) {
    if (image != NULL) {
        free(image->rgb);
        memset(image, 0, sizeof(*image));
    }
}

static ht_status load_ppm_image(char const* path, ppm_image* out_image) {
    unsigned char* data = NULL;
    size_t size = 0u;
    size_t position = 0u;
    char token[32];
    unsigned max_value;
    size_t pixel_count;
    size_t raster_size;
    ht_status status;

    if (out_image != NULL) {
        memset(out_image, 0, sizeof(*out_image));
    }
    if ((path == NULL) || (out_image == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    status = read_file(path, &data, &size);
    if (status != HT_OK) {
        return status;
    }

    status = ppm_next_token(data, size, &position, token, sizeof(token));
    if (status == HT_OK) {
        status = (strcmp(token, "P6") == 0) ? HT_OK : HT_ERR_UNSUPPORTED;
    }
    if (status == HT_OK) {
        status = ppm_next_token(data, size, &position, token, sizeof(token));
    }
    if (status == HT_OK) {
        status = parse_unsigned_token(token, &out_image->width);
    }
    if (status == HT_OK) {
        status = ppm_next_token(data, size, &position, token, sizeof(token));
    }
    if (status == HT_OK) {
        status = parse_unsigned_token(token, &out_image->height);
    }
    if (status == HT_OK) {
        status = ppm_next_token(data, size, &position, token, sizeof(token));
    }
    if (status == HT_OK) {
        status = parse_unsigned_token(token, &max_value);
    }
    if ((status == HT_OK) && ((out_image->width == 0u) || (out_image->height == 0u))) {
        status = HT_ERR_CORRUPT_DATA;
    }
    if ((status == HT_OK) && (max_value != 255u)) {
        status = HT_ERR_UNSUPPORTED;
    }
    if ((status == HT_OK) && ((position >= size) || !ppm_is_space(data[position]))) {
        status = HT_ERR_CORRUPT_DATA;
    }
    if (status == HT_OK) {
        ++position;
        if ((size_t)out_image->width > (SIZE_MAX / (size_t)out_image->height)) {
            status = HT_ERR_CORRUPT_DATA;
        }
    }
    if (status == HT_OK) {
        pixel_count = (size_t)out_image->width * (size_t)out_image->height;
        if (pixel_count > (SIZE_MAX / 3u)) {
            status = HT_ERR_CORRUPT_DATA;
        } else {
            size_t trailing_index;
            raster_size = pixel_count * 3u;
            if ((size - position) < raster_size) {
                status = HT_ERR_CORRUPT_DATA;
            } else {
                for (trailing_index = position + raster_size; trailing_index < size; ++trailing_index) {
                    if (!ppm_is_space(data[trailing_index])) {
                        status = HT_ERR_CORRUPT_DATA;
                        break;
                    }
                }
            }
        }
    }
    if (status == HT_OK) {
        out_image->rgb_size = raster_size;
        out_image->rgb = malloc(raster_size);
        if (out_image->rgb == NULL) {
            status = HT_ERR_IO;
        } else {
            memcpy(out_image->rgb, &data[position], raster_size);
        }
    }

    free(data);
    if (status != HT_OK) {
        ppm_image_destroy(out_image);
    }
    return status;
}

static bool rect_is_inside_asset(ht_rect_record rect, unsigned width, unsigned height) {
    int64_t right;
    int64_t bottom;

    if ((rect.x < 0) || (rect.y < 0) || (rect.width <= 0) || (rect.height <= 0)) {
        return false;
    }
    right = (int64_t)rect.x + (int64_t)rect.width;
    bottom = (int64_t)rect.y + (int64_t)rect.height;
    return (right <= (int64_t)width) && (bottom <= (int64_t)height);
}

static ht_rect_record fit_rect(unsigned source_width,
                               unsigned source_height,
                               unsigned viewport_width,
                               unsigned viewport_height) {
    ht_rect_record rect;
    uint64_t by_width_height = (uint64_t)viewport_width * (uint64_t)source_height;
    uint64_t by_height_width = (uint64_t)viewport_height * (uint64_t)source_width;

    memset(&rect, 0, sizeof(rect));
    if (by_width_height <= by_height_width) {
        rect.width = (int)viewport_width;
        rect.height = (int)(by_width_height / (uint64_t)source_width);
        if (rect.height <= 0) {
            rect.height = 1;
        }
    } else {
        rect.height = (int)viewport_height;
        rect.width = (int)(by_height_width / (uint64_t)source_height);
        if (rect.width <= 0) {
            rect.width = 1;
        }
    }
    rect.x = (int)((viewport_width - (unsigned)rect.width) / 2u);
    rect.y = (int)((viewport_height - (unsigned)rect.height) / 2u);
    return rect;
}

static ht_rect_record project_rect(ht_rect_record source,
                                   unsigned source_width,
                                   unsigned source_height,
                                   ht_rect_record fitted) {
    ht_rect_record projected;
    uint64_t left;
    uint64_t top;
    uint64_t right;
    uint64_t bottom;

    left = (uint64_t)(unsigned)source.x * (uint64_t)(unsigned)fitted.width
           / (uint64_t)source_width;
    top = (uint64_t)(unsigned)source.y * (uint64_t)(unsigned)fitted.height
          / (uint64_t)source_height;
    right = (uint64_t)(unsigned)(source.x + source.width) * (uint64_t)(unsigned)fitted.width
            / (uint64_t)source_width;
    bottom = (uint64_t)(unsigned)(source.y + source.height) * (uint64_t)(unsigned)fitted.height
             / (uint64_t)source_height;

    projected.x = fitted.x + (int)left;
    projected.y = fitted.y + (int)top;
    projected.width = (int)((right > left) ? (right - left) : 1u);
    projected.height = (int)((bottom > top) ? (bottom - top) : 1u);
    return projected;
}

static void set_surface_pixel(SDL_Surface* surface, int x, int y, Uint32 color) {
    Uint8* row;

    if ((surface == NULL) || (x < 0) || (y < 0) || (x >= surface->w) || (y >= surface->h)) {
        return;
    }
    row = (Uint8*)surface->pixels + ((size_t)y * (size_t)surface->pitch);
    ((Uint32*)row)[x] = color;
}

static void clear_surface(SDL_Surface* surface, Uint32 color) {
    if (surface != NULL) {
        SDL_FillRect(surface, NULL, color);
    }
}

static void render_score_pixels(SDL_Surface* surface, ppm_image const* image, ht_rect_record fitted) {
    int y;
    int x;

    if ((surface == NULL) || (image == NULL) || (image->rgb == NULL)) {
        return;
    }
    for (y = 0; y < fitted.height; ++y) {
        unsigned source_y = (unsigned)(((uint64_t)(unsigned)y * (uint64_t)image->height)
                                       / (uint64_t)(unsigned)fitted.height);
        for (x = 0; x < fitted.width; ++x) {
            unsigned source_x = (unsigned)(((uint64_t)(unsigned)x * (uint64_t)image->width)
                                           / (uint64_t)(unsigned)fitted.width);
            size_t offset = (((size_t)source_y * (size_t)image->width) + (size_t)source_x) * 3u;
            Uint32 color = SDL_MapRGB(surface->format,
                                      image->rgb[offset],
                                      image->rgb[offset + 1u],
                                      image->rgb[offset + 2u]);
            set_surface_pixel(surface, fitted.x + x, fitted.y + y, color);
        }
    }
}

static void draw_overlay_outline(SDL_Surface* surface, ht_rect_record rect) {
    Uint32 color;
    int t;
    int x;
    int y;
    int thickness = 2;

    if (surface == NULL) {
        return;
    }
    color = SDL_MapRGB(surface->format, 255u, 0u, 0u);
    if ((rect.width < 2) || (rect.height < 2)) {
        thickness = 1;
    }
    for (t = 0; t < thickness; ++t) {
        for (x = rect.x; x < (rect.x + rect.width); ++x) {
            set_surface_pixel(surface, x, rect.y + t, color);
            set_surface_pixel(surface, x, rect.y + rect.height - 1 - t, color);
        }
        for (y = rect.y; y < (rect.y + rect.height); ++y) {
            set_surface_pixel(surface, rect.x + t, y, color);
            set_surface_pixel(surface, rect.x + rect.width - 1 - t, y, color);
        }
    }
}

static ht_status write_surface_ppm(SDL_Surface* surface, char const* path) {
    FILE* file;
    int y;
    int x;

    if ((surface == NULL) || (path == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return HT_ERR_IO;
    }
    if (fprintf(file, "P6\n%d %d\n255\n", surface->w, surface->h) < 0) {
        fclose(file);
        return HT_ERR_IO;
    }
    for (y = 0; y < surface->h; ++y) {
        Uint8* row = (Uint8*)surface->pixels + ((size_t)y * (size_t)surface->pitch);
        for (x = 0; x < surface->w; ++x) {
            Uint8 r;
            Uint8 g;
            Uint8 b;
            Uint8 a;
            Uint32 pixel = ((Uint32*)row)[x];

            SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
            (void)a;
            if ((fputc((int)r, file) == EOF) || (fputc((int)g, file) == EOF)
                || (fputc((int)b, file) == EOF)) {
                fclose(file);
                return HT_ERR_IO;
            }
        }
    }
    if (fclose(file) != 0) {
        return HT_ERR_IO;
    }
    return HT_OK;
}

static ht_status render_snapshot(ht_score_renderer_view_result const* result,
                                 ppm_image const* image,
                                 char const* output_path) {
    SDL_Surface* surface;
    ht_status status;

    surface = SDL_CreateRGBSurfaceWithFormat(0,
                                             (int)result->viewport_width_px,
                                             (int)result->viewport_height_px,
                                             32,
                                             SDL_PIXELFORMAT_RGBA32);
    if (surface == NULL) {
        return HT_ERR_IO;
    }
    clear_surface(surface, SDL_MapRGB(surface->format, 34u, 38u, 46u));
    render_score_pixels(surface, image, result->fitted_score_rect);
    draw_overlay_outline(surface, result->active_overlay_rect);
    status = write_surface_ppm(surface, output_path);
    SDL_FreeSurface(surface);
    return status;
}

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

    SDL_GetVersion(&sdl_version);
    ttf_version = TTF_Linked_Version();
    if (ttf_version == NULL) {
        return HT_ERR_UNSUPPORTED;
    }

    renderer->linked_sdl_major = sdl_version.major;
    renderer->linked_ttf_major = ttf_version->major;
    return HT_OK;
}

ht_status ht_score_renderer_render_view(ht_score_renderer* renderer,
                                        ht_score_renderer_view_request const* request,
                                        ht_score_renderer_view_result* out_result) {
    ht_catalog* catalog = NULL;
    ht_overlay_store* overlays = NULL;
    ht_variant_record variant;
    ht_overlay_record overlay;
    ht_overlay_step_record step;
    char corpus_root[HT_PATH_CAPACITY];
    char asset_root[HT_PATH_CAPACITY];
    char output_path[HT_PATH_CAPACITY];
    ppm_image image;
    ht_status status;
    bool should_write_snapshot;

    if (out_result != NULL) {
        memset(out_result, 0, sizeof(*out_result));
        out_result->catalog_status = HT_OK;
        out_result->overlay_status = HT_OK;
        out_result->asset_status = HT_OK;
        out_result->render_status = HT_OK;
    }
    if ((renderer == NULL) || (request == NULL) || (out_result == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((request->viewport_width_px == 0u) || (request->viewport_height_px == 0u)
        || !request_text_present(request->corpus_root, sizeof(request->corpus_root))
        || !request_text_present(request->asset_root, sizeof(request->asset_root))
        || !request_text_present(request->variant_id, sizeof(request->variant_id))) {
        out_result->render_status = HT_ERR_INVALID_ARG;
        return HT_ERR_INVALID_ARG;
    }

    status = copy_request_text(corpus_root,
                               sizeof(corpus_root),
                               request->corpus_root,
                               sizeof(request->corpus_root));
    if (status == HT_OK) {
        status = copy_request_text(asset_root,
                                   sizeof(asset_root),
                                   request->asset_root,
                                   sizeof(request->asset_root));
    }
    if (status == HT_OK) {
        status = copy_request_text(out_result->variant_id,
                                   sizeof(out_result->variant_id),
                                   request->variant_id,
                                   sizeof(request->variant_id));
    }
    if (status != HT_OK) {
        out_result->render_status = status;
        return status;
    }
    out_result->viewport_width_px = request->viewport_width_px;
    out_result->viewport_height_px = request->viewport_height_px;

    should_write_snapshot = request_text_present(request->output_path, sizeof(request->output_path));
    if (should_write_snapshot) {
        status = copy_request_text(output_path,
                                   sizeof(output_path),
                                   request->output_path,
                                   sizeof(request->output_path));
        if (status != HT_OK) {
            out_result->render_status = status;
            return status;
        }
    } else {
        output_path[0] = '\0';
    }

    status = ht_catalog_open(&catalog, corpus_root);
    out_result->catalog_status = status;
    if (status != HT_OK) {
        out_result->render_status = status;
        return status;
    }
    status = ht_catalog_lookup_variant(catalog, out_result->variant_id, &variant);
    out_result->catalog_status = status;
    if (status != HT_OK) {
        ht_catalog_close(catalog);
        out_result->render_status = status;
        return (status == HT_ERR_NOT_FOUND) ? HT_OK : status;
    }

    status = copy_text(out_result->overlay_id, sizeof(out_result->overlay_id), variant.overlay_id);
    if (status == HT_OK) {
        status = join_path(out_result->resolved_asset_path,
                           sizeof(out_result->resolved_asset_path),
                           asset_root,
                           variant.display_asset_path);
    }
    if (status != HT_OK) {
        ht_catalog_close(catalog);
        out_result->render_status = status;
        return status;
    }
    out_result->source_asset_width_px = variant.display_asset_width_px;
    out_result->source_asset_height_px = variant.display_asset_height_px;
    if ((variant.display_asset_width_px == 0u) || (variant.display_asset_height_px == 0u)) {
        ht_catalog_close(catalog);
        out_result->catalog_status = HT_ERR_CORRUPT_DATA;
        out_result->render_status = HT_ERR_CORRUPT_DATA;
        return HT_ERR_CORRUPT_DATA;
    }
    if (variant.overlay_id[0] == '\0') {
        ht_catalog_close(catalog);
        out_result->overlay_status = HT_ERR_UNSUPPORTED;
        out_result->render_status = HT_ERR_UNSUPPORTED;
        return HT_OK;
    }

    status = ht_overlay_store_open(&overlays, corpus_root);
    out_result->overlay_status = status;
    if (status != HT_OK) {
        ht_catalog_close(catalog);
        out_result->render_status = status;
        return status;
    }
    status = ht_overlay_store_get_overlay(overlays, variant.overlay_id, &overlay);
    out_result->overlay_status = status;
    if (status != HT_OK) {
        ht_overlay_store_close(overlays);
        ht_catalog_close(catalog);
        out_result->render_status = status;
        return (status == HT_ERR_NOT_FOUND) ? HT_OK : status;
    }
    if ((overlay.reference_asset_width_px != variant.display_asset_width_px)
        || (overlay.reference_asset_height_px != variant.display_asset_height_px)) {
        ht_overlay_store_close(overlays);
        ht_catalog_close(catalog);
        out_result->overlay_status = HT_ERR_CORRUPT_DATA;
        out_result->render_status = HT_ERR_CORRUPT_DATA;
        return HT_ERR_CORRUPT_DATA;
    }

    status = ht_overlay_store_get_step(overlays, variant.overlay_id, request->step_index, &step);
    out_result->overlay_status = status;
    if (status != HT_OK) {
        ht_overlay_store_close(overlays);
        ht_catalog_close(catalog);
        out_result->render_status = status;
        return (status == HT_ERR_NOT_FOUND) ? HT_OK : status;
    }
    if (!rect_is_inside_asset(step.cursor_region,
                              variant.display_asset_width_px,
                              variant.display_asset_height_px)) {
        ht_overlay_store_close(overlays);
        ht_catalog_close(catalog);
        out_result->overlay_status = HT_ERR_CORRUPT_DATA;
        out_result->render_status = HT_ERR_CORRUPT_DATA;
        return HT_ERR_CORRUPT_DATA;
    }

    out_result->fitted_score_rect = fit_rect(variant.display_asset_width_px,
                                             variant.display_asset_height_px,
                                             request->viewport_width_px,
                                             request->viewport_height_px);
    out_result->active_overlay_rect = project_rect(step.cursor_region,
                                                   variant.display_asset_width_px,
                                                   variant.display_asset_height_px,
                                                   out_result->fitted_score_rect);

    ht_overlay_store_close(overlays);
    ht_catalog_close(catalog);

    if (!should_write_snapshot) {
        return HT_OK;
    }

    memset(&image, 0, sizeof(image));
    status = load_ppm_image(out_result->resolved_asset_path, &image);
    out_result->asset_status = status;
    if (status != HT_OK) {
        out_result->render_status = status;
        return (status == HT_ERR_NOT_FOUND) ? HT_OK : status;
    }
    if ((image.width != variant.display_asset_width_px) || (image.height != variant.display_asset_height_px)) {
        ppm_image_destroy(&image);
        out_result->asset_status = HT_ERR_CORRUPT_DATA;
        out_result->render_status = HT_ERR_CORRUPT_DATA;
        return HT_ERR_CORRUPT_DATA;
    }

    status = render_snapshot(out_result, &image, output_path);
    ppm_image_destroy(&image);
    out_result->render_status = status;
    return status;
}
