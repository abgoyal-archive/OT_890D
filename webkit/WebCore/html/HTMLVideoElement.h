

#ifndef HTMLVideoElement_h
#define HTMLVideoElement_h

#if ENABLE(VIDEO)

#include "HTMLMediaElement.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class HTMLImageLoader;

class HTMLVideoElement : public HTMLMediaElement {
public:
    HTMLVideoElement(const QualifiedName&, Document*);
    
    virtual int tagPriority() const { return 5; }
    virtual bool rendererIsNeeded(RenderStyle*);
#if !ENABLE(PLUGIN_PROXY_FOR_VIDEO)
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
#endif
    virtual void attach();
    virtual void detach();
    virtual void parseMappedAttribute(MappedAttribute* attr);
    virtual bool isVideo() const { return true; }
    virtual bool hasVideo() const { return player() && player()->hasVideo(); }
    virtual bool supportsFullscreen() const { return player() && player()->supportsFullscreen(); }
    virtual bool isURLAttribute(Attribute*) const;
    virtual const QualifiedName& imageSourceAttributeName() const;

    unsigned width() const;
    void setWidth(unsigned);
    unsigned height() const;
    void setHeight(unsigned);
    
    unsigned videoWidth() const;
    unsigned videoHeight() const;
    
    KURL poster() const;
    void setPoster(const String&);

    void updatePosterImage();

    void paint(GraphicsContext*, const IntRect&);
    // Used by canvas to gain raw pixel access
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);

private:
    OwnPtr<HTMLImageLoader> m_imageLoader;
    bool m_shouldShowPosterImage;
};

} //namespace

#endif
#endif
