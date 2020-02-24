#pragma once

#ifdef __cplusplus

#define EXTERN_C extern "C" {
#define EXTERN_C_END }

#undef ERROR
#define ERROR -1

#else

#define EXTERN_C
#define EXTERN_C_END

#endif
