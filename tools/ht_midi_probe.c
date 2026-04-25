#include <alsa/asoundlib.h>

#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <time.h>

#ifndef EX_USAGE
#define EX_USAGE 64
#endif

#ifndef HT_PROJECT_VERSION
#define HT_PROJECT_VERSION "0.0.0"
#endif

#define HT_MIDI_PROBE_MAX_DURATION_SECONDS 3600L

typedef enum ht_probe_format {
    HT_PROBE_FORMAT_NONE = 0,
    HT_PROBE_FORMAT_TABLE,
    HT_PROBE_FORMAT_TSV
} ht_probe_format;

typedef struct ht_probe_options {
    bool list_ports;
    char const* port_text;
    long duration_seconds;
    ht_probe_format format;
} ht_probe_options;

typedef struct ht_probe_context {
    snd_seq_t* seq;
    snd_seq_addr_t sender;
    int local_port;
    bool connected;
} ht_probe_context;

typedef struct ht_probe_event {
    char const* kind;
    int channel;
    bool has_channel;
    int note;
    bool has_note;
    int velocity;
    bool has_velocity;
    int controller;
    bool has_controller;
    int value;
    bool has_value;
    unsigned raw_type;
} ht_probe_event;

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
            "  ht_midi_probe --port CLIENT:PORT --duration SECONDS --format table\n"
            "  ht_midi_probe --port CLIENT:PORT --duration SECONDS --format tsv\n");
}

static int usage_error(char const* message) {
    fprintf(stderr, "usage: %s\n", message);
    print_usage(stderr);
    return EX_USAGE;
}

static void print_alsa_error(char const* action, int rc) {
    fprintf(stderr, "alsa: %s: %s\n", action, snd_strerror(rc));
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
        return EX_USAGE;
    }

    memset(out_options, 0, sizeof(*out_options));
    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--help") == 0) {
            print_usage(stdout);
            exit(0);
        }
        if (strcmp(argv[index], "--version") == 0) {
            printf("hanon-trainer %s\nALSA %s\n", HT_PROJECT_VERSION, snd_asoundlib_version());
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
            || (out_options->format != HT_PROBE_FORMAT_NONE)) {
            return usage_error("--list cannot be combined with capture options");
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
        return usage_error("capture requires --format table or --format tsv");
    }

    return 0;
}

static int open_sequencer(snd_seq_t** out_seq) {
    snd_seq_t* seq = NULL;
    int rc;

    if (out_seq != NULL) {
        *out_seq = NULL;
    }
    if (out_seq == NULL) {
        return -EINVAL;
    }

    rc = snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK);
    if (rc < 0) {
        return rc;
    }

    rc = snd_seq_set_client_name(seq, "hanon-trainer MIDI probe");
    if (rc < 0) {
        snd_seq_close(seq);
        return rc;
    }

    *out_seq = seq;
    return 0;
}

static int list_ports(void) {
    snd_seq_t* seq = NULL;
    snd_seq_client_info_t* client_info = NULL;
    snd_seq_port_info_t* port_info = NULL;
    unsigned listed_count = 0u;
    int rc;

    rc = open_sequencer(&seq);
    if (rc < 0) {
        print_alsa_error("cannot open sequencer", rc);
        return 1;
    }

    rc = snd_seq_client_info_malloc(&client_info);
    if (rc < 0) {
        print_alsa_error("cannot allocate client info", rc);
        snd_seq_close(seq);
        return 1;
    }
    rc = snd_seq_port_info_malloc(&port_info);
    if (rc < 0) {
        print_alsa_error("cannot allocate port info", rc);
        snd_seq_client_info_free(client_info);
        snd_seq_close(seq);
        return 1;
    }

    printf("port\tclient\tname\n");
    snd_seq_client_info_set_client(client_info, -1);
    while (snd_seq_query_next_client(seq, client_info) >= 0) {
        int client = snd_seq_client_info_get_client(client_info);
        char const* client_name = snd_seq_client_info_get_name(client_info);

        snd_seq_port_info_set_client(port_info, client);
        snd_seq_port_info_set_port(port_info, -1);
        while (snd_seq_query_next_port(seq, port_info) >= 0) {
            unsigned int capability = snd_seq_port_info_get_capability(port_info);

            if (((capability & SND_SEQ_PORT_CAP_READ) != 0u)
                && ((capability & SND_SEQ_PORT_CAP_SUBS_READ) != 0u)) {
                printf("%d:%d\t%s\t%s\n",
                       client,
                       snd_seq_port_info_get_port(port_info),
                       client_name,
                       snd_seq_port_info_get_name(port_info));
                ++listed_count;
            }
        }
    }

    if (listed_count == 0u) {
        fprintf(stderr, "alsa: no readable MIDI source ports found\n");
    }

    snd_seq_port_info_free(port_info);
    snd_seq_client_info_free(client_info);
    snd_seq_close(seq);
    return 0;
}

static int64_t monotonic_ms(void) {
    struct timespec now;

    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return 0;
    }

    return ((int64_t)now.tv_sec * 1000) + ((int64_t)now.tv_nsec / 1000000);
}

static void format_int_or_empty(int value, bool has_value) {
    if (has_value) {
        printf("%d", value);
    }
}

static int user_midi_channel(int alsa_channel) {
    return alsa_channel + 1;
}

static char const* raw_type_name(unsigned raw_type) {
    switch (raw_type) {
    case 0x80u:
        return "NOTEOFF";
    case 0x90u:
        return "NOTEON";
    case 0xA0u:
        return "KEYPRESS";
    case 0xB0u:
        return "CONTROLLER";
    case 0xC0u:
        return "PGMCHANGE";
    case 0xD0u:
        return "CHANPRESS";
    case 0xE0u:
        return "PITCHBEND";
    case 0xF0u:
        return "SYSEX";
    default:
        return "OTHER";
    }
}

static ht_probe_event decode_event(snd_seq_event_t const* event) {
    ht_probe_event decoded;

    memset(&decoded, 0, sizeof(decoded));
    decoded.kind = "other";
    decoded.raw_type = (event == NULL) ? 0u : (unsigned)event->type;
    if (event == NULL) {
        return decoded;
    }

    switch (event->type) {
    case SND_SEQ_EVENT_NOTEON:
        decoded.channel = user_midi_channel(event->data.note.channel);
        decoded.has_channel = true;
        decoded.note = event->data.note.note;
        decoded.has_note = true;
        decoded.velocity = event->data.note.velocity;
        decoded.has_velocity = true;
        decoded.raw_type = 0x90u;
        decoded.kind = (event->data.note.velocity == 0) ? "note_off" : "note_on";
        break;
    case SND_SEQ_EVENT_NOTEOFF:
        decoded.channel = user_midi_channel(event->data.note.channel);
        decoded.has_channel = true;
        decoded.note = event->data.note.note;
        decoded.has_note = true;
        decoded.velocity = event->data.note.velocity;
        decoded.has_velocity = true;
        decoded.raw_type = 0x80u;
        decoded.kind = "note_off";
        break;
    case SND_SEQ_EVENT_CONTROLLER:
        decoded.channel = user_midi_channel(event->data.control.channel);
        decoded.has_channel = true;
        decoded.controller = event->data.control.param;
        decoded.has_controller = true;
        decoded.value = event->data.control.value;
        decoded.has_value = true;
        decoded.raw_type = 0xB0u;
        decoded.kind = "controller";
        break;
    case SND_SEQ_EVENT_PITCHBEND:
        decoded.channel = user_midi_channel(event->data.control.channel);
        decoded.has_channel = true;
        decoded.value = event->data.control.value;
        decoded.has_value = true;
        decoded.raw_type = 0xE0u;
        decoded.kind = "pitchbend";
        break;
    case SND_SEQ_EVENT_PGMCHANGE:
        decoded.channel = user_midi_channel(event->data.control.channel);
        decoded.has_channel = true;
        decoded.value = event->data.control.value;
        decoded.has_value = true;
        decoded.raw_type = 0xC0u;
        decoded.kind = "pgmchange";
        break;
    case SND_SEQ_EVENT_CHANPRESS:
        decoded.channel = user_midi_channel(event->data.control.channel);
        decoded.has_channel = true;
        decoded.value = event->data.control.value;
        decoded.has_value = true;
        decoded.raw_type = 0xD0u;
        decoded.kind = "chanpress";
        break;
    case SND_SEQ_EVENT_KEYPRESS:
        decoded.channel = user_midi_channel(event->data.note.channel);
        decoded.has_channel = true;
        decoded.note = event->data.note.note;
        decoded.has_note = true;
        decoded.value = event->data.note.velocity;
        decoded.has_value = true;
        decoded.raw_type = 0xA0u;
        decoded.kind = "keypress";
        break;
    case SND_SEQ_EVENT_SYSEX:
        decoded.value = (int)event->data.ext.len;
        decoded.has_value = true;
        decoded.raw_type = 0xF0u;
        decoded.kind = "sysex";
        break;
    default:
        decoded.kind = "other";
        break;
    }

    return decoded;
}

static void print_table_event(int64_t ms, ht_probe_event const* event) {
    printf("%8lld  %-10s  ch=",
           (long long)ms,
           event->kind);
    if (event->has_channel) {
        printf("%d", event->channel);
    } else {
        printf("-");
    }
    printf("  note=");
    if (event->has_note) {
        printf("%d", event->note);
    } else {
        printf("-");
    }
    printf("  velocity=");
    if (event->has_velocity) {
        printf("%d", event->velocity);
    } else {
        printf("-");
    }
    printf("  controller=");
    if (event->has_controller) {
        printf("%d", event->controller);
    } else {
        printf("-");
    }
    printf("  value=");
    if (event->has_value) {
        printf("%d", event->value);
    } else {
        printf("-");
    }
    printf("  raw_type=%s(0x%02X)\n", raw_type_name(event->raw_type), event->raw_type);
}

static void print_tsv_header(void) {
    printf("ms\tkind\tchannel\tnote\tvelocity\tcontroller\tvalue\traw_type\n");
}

static void print_tsv_event(int64_t ms, ht_probe_event const* event) {
    printf("%lld\t%s\t", (long long)ms, event->kind);
    format_int_or_empty(event->channel, event->has_channel);
    printf("\t");
    format_int_or_empty(event->note, event->has_note);
    printf("\t");
    format_int_or_empty(event->velocity, event->has_velocity);
    printf("\t");
    format_int_or_empty(event->controller, event->has_controller);
    printf("\t");
    format_int_or_empty(event->value, event->has_value);
    printf("\t0x%02X\n", event->raw_type);
}

static void print_event(ht_probe_format format, int64_t ms, snd_seq_event_t const* event) {
    ht_probe_event decoded = decode_event(event);

    if (format == HT_PROBE_FORMAT_TSV) {
        print_tsv_event(ms, &decoded);
        return;
    }

    print_table_event(ms, &decoded);
}

static void close_probe_context(ht_probe_context* context) {
    if ((context == NULL) || (context->seq == NULL)) {
        return;
    }
    if (context->connected) {
        snd_seq_disconnect_from(context->seq,
                                context->local_port,
                                context->sender.client,
                                context->sender.port);
    }
    if (context->local_port >= 0) {
        snd_seq_delete_simple_port(context->seq, context->local_port);
    }
    snd_seq_close(context->seq);
    context->seq = NULL;
    context->local_port = -1;
    context->connected = false;
}

static int open_capture(ht_probe_options const* options, ht_probe_context* context) {
    int rc;

    memset(context, 0, sizeof(*context));
    context->local_port = -1;
    rc = open_sequencer(&context->seq);
    if (rc < 0) {
        print_alsa_error("cannot open sequencer", rc);
        return 1;
    }

    rc = snd_seq_parse_address(context->seq, &context->sender, options->port_text);
    if (rc < 0) {
        fprintf(stderr, "alsa: cannot parse port '%s': %s\n", options->port_text, snd_strerror(rc));
        close_probe_context(context);
        return 1;
    }

    context->local_port = snd_seq_create_simple_port(
        context->seq,
        "ht_midi_probe",
        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
        SND_SEQ_PORT_TYPE_APPLICATION);
    if (context->local_port < 0) {
        print_alsa_error("cannot create local input port", context->local_port);
        close_probe_context(context);
        return 1;
    }

    rc = snd_seq_connect_from(context->seq,
                              context->local_port,
                              context->sender.client,
                              context->sender.port);
    if (rc < 0) {
        print_alsa_error("cannot connect from source port", rc);
        close_probe_context(context);
        return 1;
    }
    context->connected = true;

    fprintf(stderr,
            "alsa: connected from %d:%d for %ld seconds\n",
            context->sender.client,
            context->sender.port,
            options->duration_seconds);
    return 0;
}

static int drain_events(ht_probe_context* context,
                        ht_probe_format format,
                        int64_t started_ms,
                        unsigned* event_count) {
    int rc;

    for (;;) {
        snd_seq_event_t* event = NULL;

        rc = snd_seq_event_input(context->seq, &event);
        if (rc == -EAGAIN) {
            return 0;
        }
        if (rc < 0) {
            print_alsa_error("cannot read event", rc);
            return 1;
        }
        if (event != NULL) {
            print_event(format, monotonic_ms() - started_ms, event);
            ++(*event_count);
            snd_seq_free_event(event);
        }
    }
}

static int capture_events(ht_probe_options const* options) {
    ht_probe_context context;
    struct pollfd* poll_descriptors = NULL;
    int descriptor_count;
    int64_t started_ms;
    int64_t deadline_ms;
    unsigned event_count = 0u;
    int result = 0;

    if (open_capture(options, &context) != 0) {
        return 1;
    }

    descriptor_count = snd_seq_poll_descriptors_count(context.seq, POLLIN);
    if (descriptor_count <= 0) {
        fprintf(stderr, "alsa: no poll descriptors available\n");
        close_probe_context(&context);
        return 1;
    }
    poll_descriptors = calloc((size_t)descriptor_count, sizeof(*poll_descriptors));
    if (poll_descriptors == NULL) {
        fprintf(stderr, "alsa: cannot allocate poll descriptors\n");
        close_probe_context(&context);
        return 1;
    }
    if (snd_seq_poll_descriptors(context.seq, poll_descriptors, (unsigned)descriptor_count, POLLIN)
        < 0) {
        fprintf(stderr, "alsa: cannot initialize poll descriptors\n");
        free(poll_descriptors);
        close_probe_context(&context);
        return 1;
    }

    signal(SIGINT, request_stop);
    signal(SIGTERM, request_stop);
    started_ms = monotonic_ms();
    deadline_ms = started_ms + (options->duration_seconds * 1000);
    if (options->format == HT_PROBE_FORMAT_TSV) {
        print_tsv_header();
    }

    while (!stop_requested) {
        int64_t now_ms = monotonic_ms();
        int64_t remaining_ms = deadline_ms - now_ms;
        int poll_timeout_ms;
        int rc;

        if (remaining_ms <= 0) {
            break;
        }
        poll_timeout_ms = (remaining_ms > INT_MAX) ? INT_MAX : (int)remaining_ms;
        rc = poll(poll_descriptors, (nfds_t)descriptor_count, poll_timeout_ms);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr, "alsa: poll failed: %s\n", strerror(errno));
            result = 1;
            break;
        }
        if (rc == 0) {
            continue;
        }
        result = drain_events(&context, options->format, started_ms, &event_count);
        if (result != 0) {
            break;
        }
    }

    if (result == 0) {
        result = drain_events(&context, options->format, started_ms, &event_count);
    }
    fflush(stdout);
    fprintf(stderr, "alsa: captured %u events in %ld seconds\n", event_count, options->duration_seconds);

    free(poll_descriptors);
    close_probe_context(&context);
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

    return capture_events(&options);
}
