

#ifndef RenderSVGBlock_h
#define RenderSVGBlock_h
#if ENABLE(SVG)

#include "RenderBlock.h"
#include "SVGRenderSupport.h"

namespace WebCore {

class SVGElement;

class RenderSVGBlock : public RenderBlock, protected SVGRenderBase {
public:
    RenderSVGBlock(SVGElement*);

private:
    virtual void setStyle(PassRefPtr<RenderStyle>);
};

}
#endif // ENABLE(SVG)
#endif // !RenderSVGBlock_h
