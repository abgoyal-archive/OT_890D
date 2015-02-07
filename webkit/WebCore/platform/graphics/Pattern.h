

#ifndef Pattern_h
#define Pattern_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include "TransformationMatrix.h"

#if PLATFORM(CG)
typedef struct CGPattern* CGPatternRef;
typedef CGPatternRef PlatformPatternPtr;
#elif PLATFORM(CAIRO)
#include <cairo.h>
typedef cairo_pattern_t* PlatformPatternPtr;
#elif PLATFORM(SKIA) || PLATFORM(SGL)
class SkShader;
typedef SkShader* PlatformPatternPtr;
#elif PLATFORM(QT)
#include <QBrush>
typedef QBrush PlatformPatternPtr;
#elif PLATFORM(WX)
#if USE(WXGC)
class wxGraphicsBrush;
typedef wxGraphicsBrush* PlatformPatternPtr;
#else
class wxBrush;
typedef wxBrush* PlatformPatternPtr;
#endif // USE(WXGC)
#elif PLATFORM(WINCE)
typedef void* PlatformPatternPtr;
#endif

namespace WebCore {
    class TransformationMatrix;
    class Image;

    class Pattern : public RefCounted<Pattern> {
    public:
        static PassRefPtr<Pattern> create(Image* tileImage, bool repeatX, bool repeatY)
        {
            return adoptRef(new Pattern(tileImage, repeatX, repeatY));
        }
        virtual ~Pattern();

        Image* tileImage() const { return m_tileImage.get(); }

        // Pattern space is an abstract space that maps to the default user space by the transformation 'userSpaceTransformation' 
        PlatformPatternPtr createPlatformPattern(const TransformationMatrix& userSpaceTransformation) const;
        void setPatternSpaceTransform(const TransformationMatrix& patternSpaceTransformation) { m_patternSpaceTransformation = patternSpaceTransformation; }

    private:
        Pattern(Image*, bool repeatX, bool repeatY);

        RefPtr<Image> m_tileImage;
        bool m_repeatX;
        bool m_repeatY;
        TransformationMatrix m_patternSpaceTransformation;
    };

} //namespace

#endif
