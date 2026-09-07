#pragma once

#ifndef MIKAN_CALL
#if defined _WIN32 || defined __CYGWIN__
#define MIKAN_CALL __cdecl
#define MIKAN_CALLBACK __stdcall
#else
#define MIKAN_CALL
#define MIKAN_CALLBACK
#endif
#endif

#ifndef MIKAN_EXTERN_C
#ifdef __cplusplus
#define MIKAN_EXTERN_C extern "C"
#else
#define MIKAN_EXTERN_C
#endif
#endif

#ifndef MIKAN_ARKIT_VIDEO_CLASS
#if defined(MIKAN_ARKIT_VIDEO_EXPORTS)
#if defined _WIN32 || defined __CYGWIN__
#define MIKAN_ARKIT_VIDEO_FUNC(rval) MIKAN_EXTERN_C __declspec(dllexport) rval MIKAN_CALL
#define MIKAN_ARKIT_VIDEO_CLASS __declspec(dllexport)
#else // Not Windows
#if __GNUC__ >= 4
#define MIKAN_ARKIT_VIDEO_FUNC(rval) MIKAN_EXTERN_C __attribute__((visibility("default"))) rval MIKAN_CALL
#define MIKAN_ARKIT_VIDEO_CLASS __attribute__((visibility("default")))
#else
#define MIKAN_ARKIT_VIDEO_FUNC(rval) MIKAN_EXTERN_C rval MIKAN_CALL
#define MIKAN_ARKIT_VIDEO_CLASS
#endif
#endif // defined _WIN32 || defined __CYGWIN__
#else
#define MIKAN_ARKIT_VIDEO_FUNC(rval) MIKAN_EXTERN_C rval MIKAN_CALL
#define MIKAN_ARKIT_VIDEO_CLASS
#endif // MIKAN_ARKIT_VIDEO_EXPORTS
#endif //! defined(MIKAN_ARKIT_VIDEO_FUNC)

#pragma once
