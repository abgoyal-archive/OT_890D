

#include <CoreGraphics/CGContext.h>

namespace WebCore {

class GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivate(CGContextRef cgContext)
        : m_cgContext(cgContext)
#if PLATFORM(WIN)
        , m_hdc(0)
        , m_transparencyCount(0)
        , m_shouldIncludeChildWindows(false)
#endif
        , m_userToDeviceTransformKnownToBeIdentity(false)
    {
        CGContextRetain(m_cgContext);
    }
    
    ~GraphicsContextPlatformPrivate()
    {
        CGContextRelease(m_cgContext);
    }

#if PLATFORM(MAC) || PLATFORM(CHROMIUM)
    // These methods do nothing on Mac.
    void save() {}
    void restore() {}
    void flush() {}
    void clip(const FloatRect&) {}
    void clip(const Path&) {}
    void scale(const FloatSize&) {}
    void rotate(float) {}
    void translate(float, float) {}
    void concatCTM(const TransformationMatrix&) {}
    void beginTransparencyLayer() {}
    void endTransparencyLayer() {}
#endif

#if PLATFORM(WIN)
    // On Windows, we need to update the HDC for form controls to draw in the right place.
    void save();
    void restore();
    void flush();
    void clip(const FloatRect&);
    void clip(const Path&);
    void scale(const FloatSize&);
    void rotate(float);
    void translate(float, float);
    void concatCTM(const TransformationMatrix&);
    void beginTransparencyLayer() { m_transparencyCount++; }
    void endTransparencyLayer() { m_transparencyCount--; }

    HDC m_hdc;
    unsigned m_transparencyCount;
    bool m_shouldIncludeChildWindows;
#endif

    CGContextRef m_cgContext;
    bool m_userToDeviceTransformKnownToBeIdentity;
};

}
