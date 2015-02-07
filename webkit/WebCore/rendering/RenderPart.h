

#ifndef RenderPart_h
#define RenderPart_h

#include "RenderWidget.h"

namespace WebCore {

class RenderPart : public RenderWidget {
public:
    RenderPart(Element*);
    virtual ~RenderPart();
    
    bool hasFallbackContent() const { return m_hasFallbackContent; }

    virtual void setWidget(PassRefPtr<Widget>);
    virtual void viewCleared();

protected:
    bool m_hasFallbackContent;

private:
    virtual bool isRenderPart() const { return true; }
    virtual const char* renderName() const { return "RenderPart"; }
};

inline RenderPart* toRenderPart(RenderObject* object)
{
    ASSERT(!object || object->isRenderPart());
    return static_cast<RenderPart*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderPart(const RenderPart*);

}

#endif
