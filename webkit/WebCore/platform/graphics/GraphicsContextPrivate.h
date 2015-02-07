

#ifndef GraphicsContextPrivate_h
#define GraphicsContextPrivate_h

#include "TransformationMatrix.h"
#include "Gradient.h"
#include "GraphicsContext.h"
#include "Pattern.h"

namespace WebCore {

    struct GraphicsContextState {
        GraphicsContextState()
            : textDrawingMode(cTextFill)
            , strokeStyle(SolidStroke)
            , strokeThickness(0)
#if PLATFORM(CAIRO)
            , globalAlpha(1.0f)
#endif
            , strokeColorSpace(SolidColorSpace)
            , strokeColor(Color::black)
            , fillRule(RULE_NONZERO)
            , fillColorSpace(SolidColorSpace)
            , fillColor(Color::black)
            , shouldAntialias(true)
            , paintingDisabled(false)
            , shadowBlur(0)
            , shadowsIgnoreTransforms(false)
        {
        }

        int textDrawingMode;
        
        StrokeStyle strokeStyle;
        float strokeThickness;
#if PLATFORM(CAIRO)
        float globalAlpha;
#elif PLATFORM(QT)
        TransformationMatrix pathTransform;
#endif
        ColorSpace strokeColorSpace;
        Color strokeColor;
        RefPtr<Gradient> strokeGradient;
        RefPtr<Pattern> strokePattern;
        
        WindRule fillRule;
        ColorSpace fillColorSpace;
        Color fillColor;
        RefPtr<Gradient> fillGradient;
        RefPtr<Pattern> fillPattern;

        bool shouldAntialias;

        bool paintingDisabled;
        
        IntSize shadowSize;
        unsigned shadowBlur;
        Color shadowColor;

        bool shadowsIgnoreTransforms;
    };

    class GraphicsContextPrivate {
    public:
        GraphicsContextPrivate()
            : m_focusRingWidth(0)
            , m_focusRingOffset(0)
            , m_updatingControlTints(false)
        {
        }

        GraphicsContextState state;
        Vector<GraphicsContextState> stack;
        Vector<IntRect> m_focusRingRects;
        int m_focusRingWidth;
        int m_focusRingOffset;
        bool m_updatingControlTints;
    };

} // namespace WebCore

#endif // GraphicsContextPrivate_h
