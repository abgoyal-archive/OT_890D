

#ifndef HTMLAreaElement_h
#define HTMLAreaElement_h

#include "HTMLAnchorElement.h"
#include "IntSize.h"

namespace WebCore {

class HitTestResult;
class Path;

class HTMLAreaElement : public HTMLAnchorElement {
public:
    HTMLAreaElement(const QualifiedName&, Document*);
    virtual ~HTMLAreaElement();

    bool isDefault() const { return m_shape == Default; }

    bool mapMouseEvent(int x, int y, const IntSize&, HitTestResult&);

    IntRect getRect(RenderObject*) const;

    KURL href() const;

    bool noHref() const;
    void setNoHref(bool);

private:
    virtual HTMLTagStatus endTagRequirement() const { return TagStatusForbidden; }
    virtual int tagPriority() const { return 0; }
    virtual void parseMappedAttribute(MappedAttribute*);
    virtual bool isFocusable() const;
    virtual String target() const;

    enum Shape { Default, Poly, Rect, Circle, Unknown };
    Path getRegion(const IntSize&) const;

    OwnPtr<Path> m_region;
    Length* m_coords;
    int m_coordsLen;
    IntSize m_lastSize;
    Shape m_shape;
};

} //namespace

#endif
