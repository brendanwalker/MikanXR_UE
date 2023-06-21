#ifndef MIKAN_CLIENT_EXPORT_H
#define MIKAN_CLIENT_EXPORT_H


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

#ifndef MIKAN_PUBLIC_FUNCTION
    #if defined(MIKAN_CLIENT_EXPORTS) || defined(MIKAN_CLIENT_CAPI_EXPORTS)  // CMake-defined when creating shared library
        #if defined _WIN32 || defined __CYGWIN__
            #define MIKAN_PUBLIC_FUNCTION(rval)       MIKAN_EXTERN_C    __declspec(dllexport)                   rval    MIKAN_CALL
            #define MIKAN_PUBLIC_CLASS                                __declspec(dllexport)
            #define MIKAN_PRIVATE_FUNCTION(rval)                                                              rval    MIKAN_CALL
            #define MIKAN_PRIVATE_CLASS
        #else  // Not Windows
            #if __GNUC__ >= 4
                #define MIKAN_PUBLIC_FUNCTION(rval)   MIKAN_EXTERN_C    __attribute__((visibility("default")))  rval    MIKAN_CALL
                #define MIKAN_PUBLIC_CLASS                            __attribute__((visibility("default")))
            #else
                #define MIKAN_PUBLIC_FUNCTION(rval)   MIKAN_EXTERN_C rval MIKAN_CALL
                #define MIKAN_PUBLIC_CLASS
            #endif
            #define MIKAN_PRIVATE_FUNCTION(rval)                      __attribute__((visibility("hidden")))   rval    MIKAN_CALL
            #define MIKAN_PRIVATE_CLASS                               __attribute__((visibility("hidden")))
        #endif  //defined _WIN32 || defined __CYGWIN__
    #elif defined(MIKAN_CLIENT_NOEXPORT) || defined(MIKAN_CLIENT_CAPI_NOEXPORT)  // Building static lib
        #define MIKAN_PUBLIC_FUNCTION(rval)           MIKAN_EXTERN_C                                            rval    MIKAN_CALL
        #define MIKAN_PUBLIC_CLASS
        #define MIKAN_PRIVATE_FUNCTION(rval)                                                                  rval    MIKAN_CALL
        #define MIKAN_PRIVATE_CLASS
    #else //This DLL/so/dylib is being imported
        #if defined _WIN32 || defined __CYGWIN__
            #define MIKAN_PUBLIC_FUNCTION(rval)       MIKAN_EXTERN_C    __declspec(dllimport)                   rval    MIKAN_CALL
            #define MIKAN_PUBLIC_CLASS                                __declspec(dllimport)
        #else  // Not Windows
            #define MIKAN_PUBLIC_FUNCTION(rval)       MIKAN_EXTERN_C                                            rval    MIKAN_CALL
            #define MIKAN_PUBLIC_CLASS
        #endif  //defined _WIN32 || defined __CYGWIN__
        #define MIKAN_PRIVATE_FUNCTION(rval)                                                                  rval    MIKAN_CALL
        #define MIKAN_PRIVATE_CLASS
    #endif //MIKAN_CLIENT_EXPORTS
#endif //!defined(MIKAN_PUBLIC_FUNCTION)

#endif // MIKAN_CLIENT_EXPORT_H
