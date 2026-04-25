#ifndef HANON_TRAINER_HT_TYPES_H
#define HANON_TRAINER_HT_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HT_ID_CAPACITY 64u
#define HT_PATH_CAPACITY 256u
#define HT_TEXT_CAPACITY 512u
#define HT_SUMMARY_CAPACITY 512u
#define HT_NOTE_SUMMARY_CAPACITY 256u
#define HT_MAX_EXPECTED_PITCHES 16u

typedef enum ht_status {
    HT_OK = 0,
    HT_ERR_INVALID_ARG,
    HT_ERR_NOT_FOUND,
    HT_ERR_IO,
    HT_ERR_DB,
    HT_ERR_UNSUPPORTED,
    HT_ERR_TIMEOUT,
    HT_ERR_EXTERNAL_TOOL,
    HT_ERR_CORRUPT_DATA,
    HT_ERR_BUFFER_TOO_SMALL
} ht_status;

typedef enum ht_support_level {
    HT_SUPPORT_ASSET_ONLY = 0,
    HT_SUPPORT_GUIDE_ONLY,
    HT_SUPPORT_PILOT_ANALYSIS,
    HT_SUPPORT_ANALYSIS_READY
} ht_support_level;

typedef enum ht_capture_status {
    HT_CAPTURE_IDLE = 0,
    HT_CAPTURE_CAPTURING,
    HT_CAPTURE_COMPLETED,
    HT_CAPTURE_ABORTED,
    HT_CAPTURE_INVALID
} ht_capture_status;

typedef enum ht_analysis_status {
    HT_ANALYSIS_PENDING = 0,
    HT_ANALYSIS_COMPLETE,
    HT_ANALYSIS_UNSUPPORTED,
    HT_ANALYSIS_FAILED
} ht_analysis_status;

typedef enum ht_generation_status {
    HT_GENERATION_COMPLETE = 0,
    HT_GENERATION_TIMEOUT,
    HT_GENERATION_FAILED,
    HT_GENERATION_STUBBED
} ht_generation_status;

typedef enum ht_midi_event_kind {
    HT_MIDI_EVENT_OTHER = 0,
    HT_MIDI_EVENT_NOTE_ON,
    HT_MIDI_EVENT_NOTE_OFF,
    HT_MIDI_EVENT_CONTROL_CHANGE
} ht_midi_event_kind;

typedef enum ht_pitch_status {
    HT_PITCH_UNCHECKED = 0,
    HT_PITCH_MATCH,
    HT_PITCH_WRONG,
    HT_PITCH_MISSED
} ht_pitch_status;

typedef enum ht_timing_status {
    HT_TIMING_UNCHECKED = 0,
    HT_TIMING_ON_TIME,
    HT_TIMING_EARLY,
    HT_TIMING_LATE,
    HT_TIMING_MISSING
} ht_timing_status;

typedef struct ht_catalog ht_catalog;
typedef struct ht_overlay_store ht_overlay_store;
typedef struct ht_session ht_session;
typedef struct ht_midi_capture ht_midi_capture;
typedef struct ht_db ht_db;
typedef struct ht_codex_cli ht_codex_cli;
typedef struct ht_score_renderer ht_score_renderer;
typedef struct ht_guidance_renderer ht_guidance_renderer;

typedef struct ht_rect_record {
    int x;
    int y;
    int width;
    int height;
} ht_rect_record;

typedef struct ht_variant_record {
    char exercise_id[HT_ID_CAPACITY];
    char variant_id[HT_ID_CAPACITY];
    unsigned exercise_number;
    char title[HT_TEXT_CAPACITY];
    char key_name[HT_ID_CAPACITY];
    unsigned sort_order;
    char tempo_text[HT_TEXT_CAPACITY];
    char repeat_text[HT_TEXT_CAPACITY];
    char practice_notes[HT_TEXT_CAPACITY];
    char source_page_path[HT_PATH_CAPACITY];
    char source_pdf_path[HT_PATH_CAPACITY];
    char display_asset_path[HT_PATH_CAPACITY];
    unsigned display_asset_width_px;
    unsigned display_asset_height_px;
    char overlay_id[HT_ID_CAPACITY];
    ht_support_level support_level;
} ht_variant_record;

typedef struct ht_overlay_record {
    char overlay_id[HT_ID_CAPACITY];
    char variant_id[HT_ID_CAPACITY];
    size_t step_count;
    char hand_scope[HT_ID_CAPACITY];
    bool analysis_enabled;
    char coverage_scope[HT_ID_CAPACITY];
    unsigned reference_asset_width_px;
    unsigned reference_asset_height_px;
    char overlay_path[HT_PATH_CAPACITY];
} ht_overlay_record;

typedef struct ht_overlay_step_record {
    char overlay_id[HT_ID_CAPACITY];
    size_t step_index;
    unsigned page_index;
    ht_rect_record cursor_region;
    char keyboard_target[HT_TEXT_CAPACITY];
    char finger_label[HT_ID_CAPACITY];
    char step_note[HT_TEXT_CAPACITY];
    int expected_pitches[HT_MAX_EXPECTED_PITCHES];
    size_t expected_pitch_count;
    int timing_window_ms;
} ht_overlay_step_record;

typedef struct ht_midi_device_record {
    char device_id[HT_ID_CAPACITY];
    char display_name[HT_TEXT_CAPACITY];
    bool is_default;
} ht_midi_device_record;

typedef struct ht_user_state_record {
    char last_variant_id[HT_ID_CAPACITY];
    size_t last_step_index;
    unsigned target_tempo;
    bool bookmark_flag;
    char user_note[HT_TEXT_CAPACITY];
    char last_midi_device_id[HT_ID_CAPACITY];
    char updated_at[HT_ID_CAPACITY];
} ht_user_state_record;

typedef struct ht_session_record {
    char session_id[HT_ID_CAPACITY];
    char variant_id[HT_ID_CAPACITY];
    char midi_device_id[HT_ID_CAPACITY];
    unsigned target_tempo;
    char started_at[HT_ID_CAPACITY];
    char ended_at[HT_ID_CAPACITY];
    unsigned duration_ms;
    ht_capture_status capture_status;
    ht_analysis_status analysis_status;
} ht_session_record;

typedef struct ht_midi_event_record {
    char session_id[HT_ID_CAPACITY];
    unsigned sequence_no;
    int64_t event_ns;
    unsigned status_byte;
    unsigned data_1;
    unsigned data_2;
    unsigned channel;
    ht_midi_event_kind event_kind;
} ht_midi_event_record;

typedef struct ht_analysis_record {
    char session_id[HT_ID_CAPACITY];
    char analyzed_at[HT_ID_CAPACITY];
    unsigned wrong_note_count;
    unsigned missed_note_count;
    unsigned extra_note_count;
    double mean_onset_error_ms;
    int max_late_ms;
    int max_early_ms;
    unsigned weak_step_count;
    char summary_text[HT_SUMMARY_CAPACITY];
} ht_analysis_record;

typedef struct ht_analysis_step_record {
    char session_id[HT_ID_CAPACITY];
    size_t step_index;
    ht_pitch_status pitch_status;
    ht_timing_status timing_status;
    char note_summary[HT_NOTE_SUMMARY_CAPACITY];
} ht_analysis_step_record;

typedef struct ht_codex_result {
    ht_generation_status generation_status;
    int exit_code;
    char output_text[HT_SUMMARY_CAPACITY];
    char error_text[HT_SUMMARY_CAPACITY];
} ht_codex_result;

typedef struct ht_advice_request_record {
    char session_id[HT_ID_CAPACITY];
    char variant_id[HT_ID_CAPACITY];
    unsigned exercise_number;
    char exercise_title[HT_TEXT_CAPACITY];
    char key_name[HT_ID_CAPACITY];
    unsigned target_tempo;
    ht_support_level support_level;
    char pitch_summary[HT_SUMMARY_CAPACITY];
    char timing_summary[HT_SUMMARY_CAPACITY];
    char weak_step_summary[HT_SUMMARY_CAPACITY];
    char user_note[HT_TEXT_CAPACITY];
} ht_advice_request_record;

typedef struct ht_advice_record {
    char advice_id[HT_ID_CAPACITY];
    char session_id[HT_ID_CAPACITY];
    char created_at[HT_ID_CAPACITY];
    char prompt_version[HT_ID_CAPACITY];
    char codex_command[HT_PATH_CAPACITY];
    char request_summary[HT_SUMMARY_CAPACITY];
    char advice_markdown[HT_SUMMARY_CAPACITY];
    ht_generation_status generation_status;
} ht_advice_record;

/**
 * Return a stable string for a status code.
 *
 * The returned pointer is static storage and must not be freed by the caller.
 */
char const* ht_status_name(ht_status status);

/**
 * Parse an on-disk support-level token such as "asset_only".
 *
 * On success, out_level is fully initialized. On failure, out_level is set to
 * HT_SUPPORT_ASSET_ONLY when the pointer is non-null.
 */
ht_status ht_support_level_from_string(char const* text, ht_support_level* out_level);

/**
 * Return the on-disk token for a support level.
 *
 * The returned pointer is static storage and must not be freed by the caller.
 */
char const* ht_support_level_name(ht_support_level level);

/**
 * Return a stable string for an advice generation status.
 *
 * The returned pointer is static storage and must not be freed by the caller.
 */
char const* ht_generation_status_name(ht_generation_status status);

#ifdef __cplusplus
}
#endif

#endif
