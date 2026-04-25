#include <hanon_trainer/ht_types.h>

#include <string.h>

char const* ht_status_name(ht_status status) {
    switch (status) {
    case HT_OK:
        return "HT_OK";
    case HT_ERR_INVALID_ARG:
        return "HT_ERR_INVALID_ARG";
    case HT_ERR_NOT_FOUND:
        return "HT_ERR_NOT_FOUND";
    case HT_ERR_IO:
        return "HT_ERR_IO";
    case HT_ERR_DB:
        return "HT_ERR_DB";
    case HT_ERR_UNSUPPORTED:
        return "HT_ERR_UNSUPPORTED";
    case HT_ERR_TIMEOUT:
        return "HT_ERR_TIMEOUT";
    case HT_ERR_EXTERNAL_TOOL:
        return "HT_ERR_EXTERNAL_TOOL";
    case HT_ERR_CORRUPT_DATA:
        return "HT_ERR_CORRUPT_DATA";
    case HT_ERR_BUFFER_TOO_SMALL:
        return "HT_ERR_BUFFER_TOO_SMALL";
    }

    return "HT_ERR_UNKNOWN";
}

ht_status ht_support_level_from_string(char const* text, ht_support_level* out_level) {
    if (out_level != NULL) {
        *out_level = HT_SUPPORT_ASSET_ONLY;
    }
    if ((text == NULL) || (out_level == NULL)) {
        return HT_ERR_INVALID_ARG;
    }

    if (strcmp(text, "asset_only") == 0) {
        *out_level = HT_SUPPORT_ASSET_ONLY;
        return HT_OK;
    }
    if (strcmp(text, "guide_only") == 0) {
        *out_level = HT_SUPPORT_GUIDE_ONLY;
        return HT_OK;
    }
    if (strcmp(text, "pilot_analysis") == 0) {
        *out_level = HT_SUPPORT_PILOT_ANALYSIS;
        return HT_OK;
    }
    if (strcmp(text, "analysis_ready") == 0) {
        *out_level = HT_SUPPORT_ANALYSIS_READY;
        return HT_OK;
    }

    return HT_ERR_CORRUPT_DATA;
}

char const* ht_support_level_name(ht_support_level level) {
    switch (level) {
    case HT_SUPPORT_ASSET_ONLY:
        return "asset_only";
    case HT_SUPPORT_GUIDE_ONLY:
        return "guide_only";
    case HT_SUPPORT_PILOT_ANALYSIS:
        return "pilot_analysis";
    case HT_SUPPORT_ANALYSIS_READY:
        return "analysis_ready";
    }

    return "asset_only";
}

char const* ht_generation_status_name(ht_generation_status status) {
    switch (status) {
    case HT_GENERATION_COMPLETE:
        return "complete";
    case HT_GENERATION_TIMEOUT:
        return "timeout";
    case HT_GENERATION_FAILED:
        return "failed";
    case HT_GENERATION_STUBBED:
        return "stubbed";
    }

    return "failed";
}
