

#ifndef RenderPartObject_h
#define RenderPartObject_h

#include "RenderPart.h"

namespace WebCore {

class RenderPartObject : public RenderPart {
public:
    RenderPartObject(Element*);
    virtual ~RenderPartObject();

    void updateWidget(bool onlyCreateNonNetscapePlugins);

private:
    virtual const char* renderName() const { return "RenderPartObject"; }

#ifdef FLATTEN_IFRAME
    virtual void calcWidth();
    virtual void calcHeight();
#endif
    virtual void layout();

    virtual void viewCleared();
};

inline RenderPartObject* toRenderPartObject(RenderObject* object)
{
    ASSERT(!object || !strcmp(object->renderName(), "RenderPartObject"));
    return static_cast<RenderPartObject*>(object);
}

// This will catch anyone doing an unnecessary cast.
void toRenderPartObject(const RenderPartObject*);

} // namespace WebCore

#endif // RenderPartObject_h
