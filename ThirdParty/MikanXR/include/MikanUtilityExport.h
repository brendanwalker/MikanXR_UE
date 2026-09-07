#pragma once

#ifndef MIKAN_UTILITY_CLASS
#if defined(MIKAN_UTILITY_EXPORTS)
#if defined _WIN32 || defined __CYGWIN__
#define MIKAN_UTILITY_FUNC(rval) __declspec(dllexport) rval
#define MIKAN_UTILITY_CLASS __declspec(dllexport)
#else // Not Windows
#if __GNUC__ >= 4
#define MIKAN_UTILITY_FUNC(rval) __attribute__((visibility("default"))) rval
#define MIKAN_UTILITY_CLASS __attribute__((visibility("default")))
#else
#define MIKAN_UTILITY_FUNC(rval) rval
#define MIKAN_UTILITY_CLASS
#endif
#endif // defined _WIN32 || defined __CYGWIN__
#else
#define MIKAN_UTILITY_FUNC(rval) rval
#define MIKAN_UTILITY_CLASS
#endif // MIKAN_UTILITY_EXPORTS
#endif //! defined(MIKAN_UTILITY_CLASS)