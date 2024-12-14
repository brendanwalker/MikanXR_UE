#pragma once

/** \def MIKAN_CALL
 * \ingroup mikan_client_misc
 * Mikan's Windows calling convention.
 *
 * Under Windows, the selection of available compilers and configurations
 * means that, unlike other platforms, there is not <em>one true calling
 * convention</em> (calling convention: the manner in which parameters are
 * passed to functions in the generated assembly code).
 *
 * Matching the Windows API itself, Mikan's client API uses the 
 * __cdecl convention and guarantees that the library is compiled in this way. 
 * The public header file also includes appropriate annotations so that 
 * your own software will use the right convention, even if another convention 
 * is being used by default within your codebase.
 *
 * On non-Windows operating systems, this macro is defined as nothing. This
 * means that you can apply it to your code without worrying about
 * cross-platform compatibility.
 */
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

#ifndef MIKAN_CORE_API
    #if defined(MIKAN_CORE_EXPORTS)
        #if defined _WIN32 || defined __CYGWIN__
            #define MIKAN_CORE_CAPI(rval)       MIKAN_EXTERN_C  __declspec(dllexport)                   rval    MIKAN_CALL
            #define MIKAN_CORE_API                              __declspec(dllexport)
        #else  // Not Windows
            #if __GNUC__ >= 4
                #define MIKAN_CORE_CAPI(rval)   MIKAN_EXTERN_C  __attribute__((visibility("default")))  rval    MIKAN_CALL
                #define MIKAN_CORE_API                          __attribute__((visibility("default")))
            #else
                #define MIKAN_CORE_CAPI(rval)   MIKAN_EXTERN_C rval MIKAN_CALL
                #define MIKAN_CORE_API
            #endif
        #endif  //defined _WIN32 || defined __CYGWIN__
    #elif defined(MIKAN_CLIENT_NOEXPORT) || defined(MIKAN_CLIENT_CAPI_NOEXPORT)  // Building static lib
        #define MIKAN_CORE_CAPI(rval)           MIKAN_EXTERN_C                                          rval    MIKAN_CALL
        #define MIKAN_CORE_API
    #else //This DLL/so/dylib is being imported
        #if defined _WIN32 || defined __CYGWIN__
            #define MIKAN_CORE_CAPI(rval)       MIKAN_EXTERN_C  __declspec(dllimport)                   rval    MIKAN_CALL
            #define MIKAN_CORE_API                              __declspec(dllimport)
        #else  // Not Windows
            #define MIKAN_CORE_CAPI(rval)       MIKAN_EXTERN_C                                          rval    MIKAN_CALL
            #define MIKAN_CORE_API
        #endif  //defined _WIN32 || defined __CYGWIN__
    #endif //MIKAN_CORE_EXPORTS
#endif //!defined(MIKAN_CORE_CAPI)

// The Enable Reflection macro is defined in the CMakeLists.txt file for modules that want to use reflection
#ifdef ENABLE_MIKANCORE_REFLECTION 
    #ifndef KODGEN_PARSING
        // IF this macro is defined it means that we both want to use reflection for MikanCore
        // and we are using it in a context where the generated reflection code is available.
        #define MIKANCORE_REFLECTION_ENABLED
    #endif
#else
    #ifndef NAMESPACE
    #define NAMESPACE(...)
    #endif

    #ifndef CLASS
    #define CLASS(...)
    #endif

    #ifndef STRUCT
    #define STRUCT(...)
    #endif

    #ifndef VARIABLE
    #define VARIABLE(...)
    #endif

    #ifndef FIELD
    #define FIELD(...)
    #endif

    #ifndef METHOD
    #define METHOD(...)
    #endif

    #ifndef ENUM
    #define ENUM(...)
    #endif

    #ifndef ENUMVALUE
    #define ENUMVALUE(...)
    #endif

    #ifndef FUNCTION
    #define FUNCTION(...)
    #endif
#endif //!defined(ENABLE_MIKANCORE_REFLECTION)