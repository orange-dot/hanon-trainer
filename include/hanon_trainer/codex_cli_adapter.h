#ifndef HANON_TRAINER_CODEX_CLI_ADAPTER_H
#define HANON_TRAINER_CODEX_CLI_ADAPTER_H

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open the local Codex CLI adapter boundary.
 *
 * Strings are borrowed during the call and copied into the handle when needed.
 * On success, out_cli receives ownership of a non-null handle.
 */
ht_status ht_codex_cli_open(ht_codex_cli** out_cli,
                            char const* executable_path,
                            char const* model_override,
                            unsigned timeout_ms);

/**
 * Release a Codex CLI adapter handle.
 *
 * Passing NULL is allowed.
 */
void ht_codex_cli_close(ht_codex_cli* cli);

#ifdef __cplusplus
}
#endif

#endif
