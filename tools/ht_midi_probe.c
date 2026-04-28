#include "midi_decode.h"
#include "midi_hal.h"

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define HT_EX_USAGE 64
#define HT_MIDI_PROBE_MAX_DURATION_SECONDS 3600L
#define HT_PROBE_REPLAY_LINE_CAPACITY 4096u
#define HT_PROBE_REPLAY_RAW_CAPACITY 256u
#define HT_PROBE_MAX_PORTS 128u

#ifndef HT_PROJECT_VERSION
#define HT_PROJECT_VERSION "0.0.0"
#endif

typedef enum ht_probe_format {
    HT_PROBE_FORMAT_NONE = 0,
    HT_PROBE_FORMAT_TABLE,
    HT_PROBE_FORMAT_TSV
} ht_probe_format;

typedef struct ht_probe_options {
    bool list_ports;
    char const* port_text;
    char const* replay_raw_tsv_path;
    long duration_seconds;
    ht_probe_format format;
} ht_probe_options;

static volatile sig_atomic_t stop_requested;

static void request_stop(int signal_number) {
    (void)signal_number;
    stop_requested = 1;
}

static void print_usage(FILE* output) {
    fprintf(output,
            "Usage:\n"
            "  ht_midi_probe --help\n"
            "  ht_midi_probe --version\n"
            "  ht_midi_probe --list\n"
            "  ht_midi_probe --port DEVICE_ID --duration SECONDS --format table\n"
            "  ht_midi_probe --port DEVICE_ID --duration SECONDS --format tsv\n"
            "  ht_midi_probe --replay-raw-tsv PATH --format tsv\n");
}

static int usage_error(char const* message) {
    fprintf(stderr, "usage: %s\n", message);
    print_usage(stderr);
    return HT_EX_USAGE;
}

static char const* probe_status_name(ht_status status) {
    switch (status) {
    case HT_OK:
        return "HT_OK";
    case HT_ERR_INVALID_ARG:
        return "HT_ERR_INVALID_ARG";
    case HT_ERR_NOT_FOUND:
        return "HT_ERR_NOT_FOUND";
    case HT_ERR_IO:
        return "HT_ERR_IO";
    case HT_ERR_UNSUPPORTED:
        return "HT_ERR_UNSUPPORTED";
    case HT_ERR_TIMEOUT:
        return "HT_ERR_TIMEOUT";
    case HT_ERR_CORRUPT_DATA:
        return "HT_ERR_CORRUPT_DATA";
    case HT_ERR_BUFFER_TOO_SMALL:
        return "HT_ERR_BUFFER_TOO_SMALL";
    case HT_ERR_DB:
    case HT_ERR_EXTERNAL_TOOL:
        return "HT_ERR_OTHER";
    }
    return "HT_ERR_OTHER";
}

static int midi_runtime_error(char const* action, ht_status status) {
    fprintf(stderr,
            "midi: %s (%s backend): %s\n",
            action,
            ht_midi_hal_backend_name(),
            probe_status_name(status));
    return 1;
}

static bool parse_long(char const* text, long* out_value) {
    char* end = NULL;
    long value;

    if ((text == NULL) || (out_value == NULL) || (text[0] == '\0') || (text[0] == '-')) {
        return false;
    }

    errno = 0;
    value = strtol(text, &end, 10);
    if ((errno != 0) || (end == text) || (*end != '\0')) {
        return false;
    }

    *out_value = value;
    return true;
}

static ht_probe_format parse_format(char const* text) {
    if (text == NULL) {
        return HT_PROBE_FORMAT_NONE;
    }
    if (strcmp(text, "table") == 0) {
        return HT_PROBE_FORMAT_TABLE;
    }
    if (strcmp(text, "tsv") == 0) {
        return HT_PROBE_FORMAT_TSV;
    }
    return HT_PROBE_FORMAT_NONE;
}

static int parse_options(int argc, char** argv, ht_probe_options* out_options) {
    int index;

    if (out_options == NULL) {
        return HT_EX_USAGE;
    }

    memset(out_options, 0, sizeof(*out_options));
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--help") == 0) {
            print_usage(stdout);
            exit(0);
        }
        if (strcmp(argv[index], "--version") == 0) {
            printf("hanon-trainer %s\nMIDI backend: %s\n",
                   HT_PROJECT_VERSION,
                   ht_midi_hal_backend_name());
            exit(0);
        }
        if (strcmp(argv[index], "--list") == 0) {
            out_options->list_ports = true;
            continue;
        }
        if (strcmp(argv[index], "--port") == 0) {
            if ((index + 1) >= argc) {
                return usage_error("--port requires a value");
            }
            ++index;
            out_options->port_text = argv[index];
            continue;
        }
        if (strcmp(argv[index], "--replay-raw-tsv") == 0) {
            if ((index + 1) >= argc) {
                return usage_error("--replay-raw-tsv requires a value");
            }
            ++index;
            out_options->replay_raw_tsv_path = argv[index];
            continue;
        }
        if (strcmp(argv[index], "--duration") == 0) {
            long duration;

            if ((index + 1) >= argc) {
                return usage_error("--duration requires a value");
            }
            ++index;
            if (!parse_long(argv[index], &duration)) {
                return usage_error("--duration must be an integer number of seconds");
            }
            if ((duration < 1L) || (duration > HT_MIDI_PROBE_MAX_DURATION_SECONDS)) {
                return usage_error("--duration must be in the range 1..3600");
            }
            out_options->duration_seconds = duration;
            continue;
        }
        if (strcmp(argv[index], "--format") == 0) {
            if ((index + 1) >= argc) {
                return usage_error("--format requires a value");
            }
            ++index;
            out_options->format = parse_format(argv[index]);
            if (out_options->format == HT_PROBE_FORMAT_NONE) {
                return usage_error("--format must be table or tsv");
            }
            continue;
        }

        return usage_error("unknown option");
    }

    if (out_options->list_ports) {
        if ((out_options->port_text != NULL) || (out_options->duration_seconds != 0L)
            || (out_options->format != HT_PROBE_FORMAT_NONE)
            || (out_options->replay_raw_tsv_path != NULL)) {
            return usage_error("--list cannot be combined with capture options");
        }
        return 0;
    }

    if (out_options->replay_raw_tsv_path != NULL) {
        if ((out_options->port_text != NULL) || (out_options->duration_seconds != 0L)) {
            return usage_error("--replay-raw-tsv cannot be combined with capture options");
        }
        if (out_options->format == HT_PROBE_FORMAT_NONE) {
            return usage_error("--replay-raw-tsv requires --format table or --format tsv");
        }
        return 0;
    }

    if (out_options->port_text == NULL) {
        return usage_error("capture requires --port");
    }
    if (out_options->duration_seconds == 0L) {
        return usage_error("capture requires --duration");
    }
    if (out_options->format == HT_PROBE_FORMAT_NONE) {
        return usage_error("capture requires --format table or tsv");
    }

    return 0;
}

static int64_t monotonic_ms(void) {
#ifdef _WIN32
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&frequency);
    }
    QueryPerformanceCounter(&counter);
    return (int64_t)((counter.QuadPart * 1000LL) / frequency.QuadPart);
#else
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }

    return ((int64_t)now.tv_sec * 1000) + ((int64_t)now.tv_nsec / 1000000);
#endif
}

static void format_unsigned_or_empty(unsigned value, bool has_value) {
    if (has_value) {
        printf("%u", value);
    }
}

static void format_int_or_empty(int value, bool has_value) {
    if (has_value) {
        printf("%d", value);
    }
}

static void print_table_event(int64_t ms, ht_midi_decoded_event const* event) {
    printf("%8lld  %-10s  ch=",
           (long long)ms,
           ht_midi_decoded_kind_name(event->kind));
    if (event->has_channel) {
        printf("%u", event->channel);
    } else {
        printf("-");
    }
    printf("  note=");
    if (event->has_note) {
        printf("%u", event->note);
    } else {
        printf("-");
    }
    printf("  velocity=");
    if (event->has_velocity) {
        printf("%u", event->velocity);
    } else {
        printf("-");
    }
    printf("  controller=");
    if (event->has_controller) {
        printf("%u", event->controller);
    } else {
        printf("-");
    }
    printf("  value=");
    if (event->has_value) {
        printf("%d", event->value);
    } else {
        printf("-");
    }
    printf("  raw_type=%s(0x%02X)\n",
           ht_midi_raw_type_name(event->raw_type),
           event->raw_type);
}

static void print_tsv_header(void) {
    printf("ms\tkind\tchannel\tnote\tvelocity\tcontroller\tvalue\traw_type\n");
}

static void print_tsv_event(int64_t ms, ht_midi_decoded_event const* event) {
    printf("%lld\t%s\t", (long long)ms, ht_midi_decoded_kind_name(event->kind));
    format_unsigned_or_empty(event->channel, event->has_channel);
    printf("\t");
    format_unsigned_or_empty(event->note, event->has_note);
    printf("\t");
    format_unsigned_or_empty(event->velocity, event->has_velocity);
    printf("\t");
    format_unsigned_or_empty(event->controller, event->has_controller);
    printf("\t");
    format_int_or_empty(event->value, event->has_value);
    printf("\t0x%02X\n", event->raw_type);
}

static void print_decoded_event(ht_probe_format format,
                                int64_t ms,
                                ht_midi_decoded_event const* decoded) {
    if (format == HT_PROBE_FORMAT_TSV) {
        print_tsv_event(ms, decoded);
        return;
    }

    print_table_event(ms, decoded);
}

static ht_status decode_hal_event(ht_midi_hal_event const* event,
                                  ht_midi_decoded_event* out_decoded) {
    if ((event == NULL) || (out_decoded == NULL)) {
        return HT_ERR_INVALID_ARG;
    }
    if ((event->raw_byte_count > 0u) && (event->raw_bytes[0] == 0xF0u)
        && (event->sysex_byte_count > 0u)) {
        return ht_midi_decode_sysex_length(event->sysex_byte_count, out_decoded);
    }
    return ht_midi_decode_raw_event(event->raw_bytes, event->raw_byte_count, out_decoded);
}

static int print_hal_event(ht_probe_format format, ht_midi_hal_event const* event) {
    ht_midi_decoded_event decoded;
    ht_status status;

    status = decode_hal_event(event, &decoded);
    if (status != HT_OK) {
        fprintf(stderr, "midi: cannot decode event: %s\n", probe_status_name(status));
        return 1;
    }

    print_decoded_event(format, event->event_ns / 1000000, &decoded);
    return 0;
}

static void trim_line_end(char* line) {
    size_t length;

    if (line == NULL) {
        return;
    }
    length = strlen(line);
    while ((length > 0u) && ((line[length - 1u] == '\n') || (line[length - 1u] == '\r'))) {
        line[length - 1u] = '\0';
        --length;
    }
}

static bool parse_hex_byte(char const* text, unsigned char* out_byte) {
    char* end = NULL;
    unsigned long value;

    if ((text == NULL) || (out_byte == NULL) || (text[0] == '\0') || (text[0] == '-')) {
        return false;
    }

    errno = 0;
    value = strtoul(text, &end, 16);
    if ((errno != 0) || (end == text) || (*end != '\0') || (value > 0xFFul)) {
        return false;
    }

    *out_byte = (unsigned char)value;
    return true;
}

static bool parse_raw_bytes(char* text,
                            unsigned char* out_bytes,
                            size_t capacity,
                            size_t* out_count) {
    char* cursor = text;
    size_t count = 0u;

    if ((text == NULL) || (out_bytes == NULL) || (out_count == NULL)) {
        return false;
    }

    while (*cursor != '\0') {
        char* token;

        while (*cursor == ' ') {
            ++cursor;
        }
        if (*cursor == '\0') {
            break;
        }

        token = cursor;
        while ((*cursor != '\0') && (*cursor != ' ')) {
            ++cursor;
        }
        if (*cursor != '\0') {
            *cursor = '\0';
            ++cursor;
        }

        if ((count >= capacity) || !parse_hex_byte(token, &out_bytes[count])) {
            return false;
        }
        ++count;
    }

    if (count == 0u) {
        return false;
    }
    *out_count = count;
    return true;
}

static bool parse_replay_line(char* line,
                              int64_t* out_ms,
                              unsigned char* out_bytes,
                              size_t capacity,
                              size_t* out_count) {
    char* tab;
    long ms;

    if ((line == NULL) || (out_ms == NULL) || (out_bytes == NULL) || (out_count == NULL)) {
        return false;
    }

    tab = strchr(line, '\t');
    if ((tab == NULL) || (strchr(tab + 1, '\t') != NULL)) {
        return false;
    }
    *tab = '\0';
    if (!parse_long(line, &ms)) {
        return false;
    }
    if (!parse_raw_bytes(tab + 1, out_bytes, capacity, out_count)) {
        return false;
    }

    *out_ms = (int64_t)ms;
    return true;
}

static int replay_raw_tsv(ht_probe_options const* options) {
    FILE* input;
    char line[HT_PROBE_REPLAY_LINE_CAPACITY];
    unsigned char raw_bytes[HT_PROBE_REPLAY_RAW_CAPACITY];
    unsigned event_count = 0u;
    unsigned line_number = 1u;

    input = fopen(options->replay_raw_tsv_path, "r");
    if (input == NULL) {
        fprintf(stderr,
                "replay: cannot open '%s': %s\n",
                options->replay_raw_tsv_path,
                strerror(errno));
        return 1;
    }

    if (fgets(line, sizeof(line), input) == NULL) {
        fprintf(stderr, "replay: missing header in '%s'\n", options->replay_raw_tsv_path);
        fclose(input);
        return 1;
    }
    trim_line_end(line);
    if (strcmp(line, "ms\traw") != 0) {
        fprintf(stderr, "replay: invalid header in '%s'\n", options->replay_raw_tsv_path);
        fclose(input);
        return 1;
    }

    if (options->format == HT_PROBE_FORMAT_TSV) {
        print_tsv_header();
    }

    while (fgets(line, sizeof(line), input) != NULL) {
        ht_midi_decoded_event decoded;
        ht_status status;
        int64_t ms;
        size_t raw_byte_count;

        ++line_number;
        trim_line_end(line);
        if (!parse_replay_line(line,
                               &ms,
                               raw_bytes,
                               sizeof(raw_bytes) / sizeof(raw_bytes[0]),
                               &raw_byte_count)) {
            fprintf(stderr,
                    "replay: invalid raw TSV line %u in '%s'\n",
                    line_number,
                    options->replay_raw_tsv_path);
            fclose(input);
            return 1;
        }

        status = ht_midi_decode_raw_event(raw_bytes, raw_byte_count, &decoded);
        if (status != HT_OK) {
            fprintf(stderr,
                    "replay: cannot decode line %u in '%s': %s\n",
                    line_number,
                    options->replay_raw_tsv_path,
                    probe_status_name(status));
            fclose(input);
            return 1;
        }

        print_decoded_event(options->format, ms, &decoded);
        ++event_count;
    }

    if (ferror(input)) {
        fprintf(stderr, "replay: read failed for '%s'\n", options->replay_raw_tsv_path);
        fclose(input);
        return 1;
    }

    fclose(input);
    fflush(stdout);
    fprintf(stderr, "replay: decoded %u events from %s\n", event_count, options->replay_raw_tsv_path);
    return 0;
}

static int list_ports(void) {
    ht_midi_hal* hal = NULL;
    ht_midi_device_record ports[HT_PROBE_MAX_PORTS];
    size_t port_count = 0u;
    size_t index;
    ht_status status;

    status = ht_midi_hal_open(&hal);
    if (status != HT_OK) {
        return midi_runtime_error("cannot open MIDI backend", status);
    }
    status = ht_midi_hal_list_ports(hal, ports, HT_PROBE_MAX_PORTS, &port_count);
    if (status != HT_OK) {
        ht_midi_hal_close(hal);
        return midi_runtime_error("cannot list MIDI ports", status);
    }

    printf("device_id\tbackend\tname\n");
    for (index = 0u; index < port_count; ++index) {
        printf("%s\t%s\t%s\n",
               ports[index].device_id,
               ht_midi_hal_backend_name(),
               ports[index].display_name);
    }
    if (port_count == 0u) {
        fprintf(stderr, "midi: no readable MIDI input ports found\n");
    }

    ht_midi_hal_close(hal);
    return 0;
}

static int drain_events(ht_midi_hal* hal, ht_probe_format format, unsigned* event_count) {
    ht_status status;

    for (;;) {
        ht_midi_hal_event event;
        bool has_event = false;

        status = ht_midi_hal_poll_event(hal, &event, &has_event);
        if (status != HT_OK) {
            return midi_runtime_error("cannot read event", status);
        }
        if (!has_event) {
            return 0;
        }
        if (print_hal_event(format, &event) != 0) {
            return 1;
        }
        ++(*event_count);
    }
}

static int capture_events(ht_probe_options const* options) {
    ht_midi_hal* hal = NULL;
    int64_t deadline_ms;
    unsigned event_count = 0u;
    int result = 0;
    ht_status status;

    status = ht_midi_hal_open(&hal);
    if (status != HT_OK) {
        return midi_runtime_error("cannot open MIDI backend", status);
    }
    status = ht_midi_hal_connect(hal, options->port_text);
    if (status != HT_OK) {
        ht_midi_hal_close(hal);
        return midi_runtime_error("cannot connect MIDI port", status);
    }

    fprintf(stderr,
            "midi: connected to %s with %s backend for %ld seconds\n",
            options->port_text,
            ht_midi_hal_backend_name(),
            options->duration_seconds);

    stop_requested = 0;
    signal(SIGINT, request_stop);
#ifdef SIGTERM
    signal(SIGTERM, request_stop);
#endif

    deadline_ms = monotonic_ms() + (options->duration_seconds * 1000);
    if (options->format == HT_PROBE_FORMAT_TSV) {
        print_tsv_header();
    }

    while (!stop_requested) {
        int64_t now_ms = monotonic_ms();
        int64_t remaining_ms = deadline_ms - now_ms;
        int wait_ms;

        if (remaining_ms <= 0) {
            break;
        }
        wait_ms = (remaining_ms > INT_MAX) ? INT_MAX : (int)remaining_ms;
        status = ht_midi_hal_wait_event(hal, wait_ms);
        if (status != HT_OK) {
            result = midi_runtime_error("cannot wait for MIDI event", status);
            break;
        }
        result = drain_events(hal, options->format, &event_count);
        if (result != 0) {
            break;
        }
    }

    if (result == 0) {
        result = drain_events(hal, options->format, &event_count);
    }
    fflush(stdout);
    fprintf(stderr, "midi: captured %u events in %ld seconds\n", event_count, options->duration_seconds);

    (void)ht_midi_hal_disconnect(hal);
    ht_midi_hal_close(hal);
    return result;
}

int main(int argc, char** argv) {
    ht_probe_options options;
    int rc;

    setlocale(LC_NUMERIC, "C");
    setvbuf(stdout, NULL, _IOLBF, 0);

    rc = parse_options(argc, argv, &options);
    if (rc != 0) {
        return rc;
    }
    if (options.list_ports) {
        return list_ports();
    }
    if (options.replay_raw_tsv_path != NULL) {
        return replay_raw_tsv(&options);
    }

    return capture_events(&options);
}
