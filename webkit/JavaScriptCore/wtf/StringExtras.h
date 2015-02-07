

#ifndef WTF_StringExtras_h
#define WTF_StringExtras_h

#include <stdarg.h>
#include <stdio.h>

#if HAVE(STRINGS_H) 
#include <strings.h> 
#endif 

#if COMPILER(MSVC)

inline int snprintf(char* buffer, size_t count, const char* format, ...) 
{
    int result;
    va_list args;
    va_start(args, format);
    result = _vsnprintf(buffer, count, format, args);
    va_end(args);
    return result;
}

#if COMPILER(MSVC7) || PLATFORM(WINCE)

inline int vsnprintf(char* buffer, size_t count, const char* format, va_list args)
{
    return _vsnprintf(buffer, count, format, args);
}

#endif

#if PLATFORM(WINCE)

inline int strnicmp(const char* string1, const char* string2, size_t count)
{
    return _strnicmp(string1, string2, count);
}

inline int stricmp(const char* string1, const char* string2)
{
    return _stricmp(string1, string2);
}

inline char* strdup(const char* strSource)
{
    return _strdup(strSource);
}

#endif

inline int strncasecmp(const char* s1, const char* s2, size_t len)
{
    return strnicmp(s1, s2, len);
}

inline int strcasecmp(const char* s1, const char* s2)
{
    return stricmp(s1, s2);
}

#endif

#endif // WTF_StringExtras_h
