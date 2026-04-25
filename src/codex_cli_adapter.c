#include <hanon_trainer/codex_cli_adapter.h>

#include <stdlib.h>
#include <string.h>

struct ht_codex_cli {
    char executable_path[HT_PATH_CAPACITY];
    char model_override[HT_ID_CAPACITY];
    unsigned timeout_ms;
};

static ht_status copy_optional(char* destination, size_t capacity, char const* source) {
    size_t length;

    if ((destination == NULL) || (capacity == 0u)) {
        return HT_ERR_INVALID_ARG;
    }
    destination[0] = '\0';
    if (source == NULL) {
        return HT_OK;
    }
    length = strlen(source);
    if (length >= capacity) {
        return HT_ERR_BUFFER_TOO_SMALL;
    }
    memcpy(destination, source, length + 1u);
    return HT_OK;
}

ht_status ht_codex_cli_open(ht_codex_cli** out_cli,
                            char const* executable_path,
                            char const* model_override,
                            unsigned timeout_ms) {
    ht_codex_cli* cli;
    ht_status status;

    if (out_cli != NULL) {
        *out_cli = NULL;
    }
    if ((out_cli == NULL) || (executable_path == NULL) || (timeout_ms == 0u)) {
        return HT_ERR_INVALID_ARG;
    }

    cli = calloc(1u, sizeof(*cli));
    if (cli == NULL) {
        return HT_ERR_IO;
    }

    status = copy_optional(cli->executable_path, sizeof(cli->executable_path), executable_path);
    if (status == HT_OK) {
        status = copy_optional(cli->model_override, sizeof(cli->model_override), model_override);
    }
    if (status != HT_OK) {
        ht_codex_cli_close(cli);
        return status;
    }

    cli->timeout_ms = timeout_ms;
    *out_cli = cli;
    return HT_OK;
}

void ht_codex_cli_close(ht_codex_cli* cli) {
    free(cli);
}
