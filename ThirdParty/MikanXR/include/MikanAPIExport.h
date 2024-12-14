#pragma once

#ifndef MIKAN_API
    #if defined(MIKAN_API_EXPORTS) 
        #if defined _WIN32 || defined __CYGWIN__
            #define MIKAN_API                                __declspec(dllexport)
        #else  // Not Windows
            #if __GNUC__ >= 4
                #define MIKAN_API                            __attribute__((visibility("default")))
            #else
                #define MIKAN_API
            #endif
        #endif  //defined _WIN32 || defined __CYGWIN__
    #elif defined(MIKAN_CLIENT_NOEXPORT) || defined(MIKAN_CLIENT_CAPI_NOEXPORT)  // Building static lib
        #define MIKAN_API
    #else //This DLL/so/dylib is being imported
        #if defined _WIN32 || defined __CYGWIN__
            #define MIKAN_API                                __declspec(dllimport)
        #else  // Not Windows
            #define MIKAN_API
        #endif  //defined _WIN32 || defined __CYGWIN__
    #endif //MIKAN_API_EXPORTS
#endif //!defined(MIKAN_API_FUNCTION)

// The Enable Reflection macro is defined in the CMakeLists.txt file for modules that want to use reflection
#ifdef ENABLE_MIKANAPI_REFLECTION 
    #ifndef KODGEN_PARSING
        // IF this macro is defined it means that we both want to use reflection for MikanCore
        // and we are using it in a context where the generated reflection code is available.
        #define MIKANAPI_REFLECTION_ENABLED
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
#endif //!defined(ENABLE_MIKANAPI_REFLECTION)