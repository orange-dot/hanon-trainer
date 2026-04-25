#ifndef HANON_TRAINER_INTERNAL_C23_FALLBACK_H
#define HANON_TRAINER_INTERNAL_C23_FALLBACK_H

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 202311L)
#define HT_NODISCARD [[nodiscard]]
#else
#define HT_NODISCARD
#endif

#endif
