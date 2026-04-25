#ifndef HANON_TRAINER_CONTENT_CATALOG_H
#define HANON_TRAINER_CONTENT_CATALOG_H

#include <hanon_trainer/ht_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open a read-only catalog from corpus_root.
 *
 * On success, stores a non-null catalog in out_catalog and transfers ownership
 * to the caller. On failure, stores NULL when out_catalog is non-null.
 */
ht_status ht_catalog_open(ht_catalog** out_catalog, char const* corpus_root);

/**
 * Release a catalog opened by ht_catalog_open.
 *
 * Passing NULL is allowed.
 */
void ht_catalog_close(ht_catalog* catalog);

/**
 * Look up one catalog variant by borrowed NUL-terminated variant_id.
 *
 * On success, out_variant is fully initialized. On failure, out_variant is
 * zeroed when non-null.
 */
ht_status ht_catalog_lookup_variant(ht_catalog const* catalog,
                                    char const* variant_id,
                                    ht_variant_record* out_variant);

#ifdef __cplusplus
}
#endif

#endif
