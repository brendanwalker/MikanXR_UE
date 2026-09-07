#pragma once

#ifndef MIKAN_COREAPP_CLASS
#if defined(MIKAN_COREAPP_EXPORTS)
#if defined _WIN32 || defined __CYGWIN__
#define MIKAN_COREAPP_FUNC(rval) __declspec(dllexport) rval
#define MIKAN_COREAPP_CLASS __declspec(dllexport)
#else // Not Windows
#if __GNUC__ >= 4
#define MIKAN_COREAPP_FUNC(rval) __attribute__((visibility("default"))) rval
#define MIKAN_COREAPP_CLASS __attribute__((visibility("default")))
#else
#define MIKAN_COREAPP_FUNC(rval) rval
#define MIKAN_COREAPP_CLASS
#endif
#endif // defined _WIN32 || defined __CYGWIN__
#else
#define MIKAN_COREAPP_FUNC(rval) rval
#define MIKAN_COREAPP_CLASS
#endif // MIKAN_COREAPP_EXPORTS
#endif //! defined(MIKAN_COREAPP_CLASS)