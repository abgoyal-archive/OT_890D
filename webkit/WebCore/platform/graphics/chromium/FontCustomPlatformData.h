

#ifndef FontCustomPlatformData_h
#define FontCustomPlatformData_h

#include "FontRenderingMode.h"
#include <wtf/Noncopyable.h>

#if PLATFORM(WIN_OS)
#include "PlatformString.h"
#include <windows.h>
#elif PLATFORM(LINUX)
#include "SkTypeface.h"
#endif

namespace WebCore {

class FontPlatformData;
class SharedBuffer;

struct FontCustomPlatformData : Noncopyable {
#if PLATFORM(WIN_OS)
    FontCustomPlatformData(HANDLE fontReference, const String& name)
        : m_fontReference(fontReference)
        , m_name(name)
    {}
#elif PLATFORM(LINUX)
    explicit FontCustomPlatformData(SkTypeface* typeface)
        : m_fontReference(typeface)
    {}
#endif

    ~FontCustomPlatformData();

    FontPlatformData fontPlatformData(int size, bool bold, bool italic,
                                      FontRenderingMode = NormalRenderingMode);

#if PLATFORM(WIN_OS)
    HANDLE m_fontReference;
    String m_name;
#elif PLATFORM(LINUX)
    SkTypeface* m_fontReference;
#endif
};

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer*);
}

#endif // FontCustomPlatformData_h
