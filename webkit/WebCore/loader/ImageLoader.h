

#ifndef ImageLoader_h
#define ImageLoader_h

#include "AtomicString.h"
#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"

namespace WebCore {

class Element;
class ImageLoadEventSender;

class ImageLoader : public CachedResourceClient {
public:
    ImageLoader(Element*);
    virtual ~ImageLoader();

    // This function should be called when the element is attached to a document; starts
    // loading if a load hasn't already been started.
    void updateFromElement();

    // This function should be called whenever the 'src' attribute is set, even if its value
    // doesn't change; starts new load unconditionally (matches Firefox and Opera behavior).
    void updateFromElementIgnoringPreviousError();

    Element* element() const { return m_element; }
    bool imageComplete() const { return m_imageComplete; }

    CachedImage* image() const { return m_image.get(); }
    void setImage(CachedImage*);

    void setLoadManually(bool loadManually) { m_loadManually = loadManually; }

    bool haveFiredLoadEvent() const { return m_firedLoad; }

    static void dispatchPendingLoadEvents();

protected:
    virtual void notifyFinished(CachedResource*);

private:
    virtual void dispatchLoadEvent() = 0;
    virtual String sourceURI(const AtomicString&) const = 0;

    friend class ImageLoadEventSender;
    void dispatchPendingLoadEvent();

    void setLoadingImage(CachedImage*);

    Element* m_element;
    CachedResourceHandle<CachedImage> m_image;
    AtomicString m_failedLoadURL;
    bool m_firedLoad : 1;
    bool m_imageComplete : 1;
    bool m_loadManually : 1;
};

}

#endif
