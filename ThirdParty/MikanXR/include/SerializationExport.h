#pragma once

#ifndef SERIALIZATION_API
    #if defined(SERIALIZATION_EXPORTS) // CMake-defined when creating shared library
        #if defined _WIN32 || defined __CYGWIN__
            #define SERIALIZATION_API          __declspec(dllexport)
        #else  // Not Windows
            #if __GNUC__ >= 4
                #define SERIALIZATION_API      __attribute__((visibility("default")))
            #else
            #define SERIALIZATION_API
            #endif
        #endif  //defined _WIN32 || defined __CYGWIN__
    #else //This DLL/so/dylib is being imported
        #if defined _WIN32 || defined __CYGWIN__
            #define SERIALIZATION_API          __declspec(dllimport)
        #else  // Not Windows
            #define SERIALIZATION_API
        #endif  //defined _WIN32 || defined __CYGWIN__
    #endif //SERIALIZATION_EXPORTS
#endif //!defined(SERIALIZATION_API)

// The Enable Reflection macro is defined in the CMakeLists.txt file for modules that want to use reflection
#ifdef ENABLE_SERIALIZATION_REFLECTION 
    #ifndef KODGEN_PARSING
        // IF this macro is defined it means that we both want to use reflection for Serialization Library
        // and we are using it in a context where the generated reflection code is available.
        #define SERIALIZATION_REFLECTION_ENABLED
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
#endif //!defined(ENABLE_SERIALIZATION_REFLECTION)