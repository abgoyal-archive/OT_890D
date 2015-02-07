

#ifndef Media_h
#define Media_h

#include "DOMWindow.h"

namespace WebCore {

class Media : public RefCounted<Media> {
public:
    static PassRefPtr<Media> create(DOMWindow* window)
    {
        return adoptRef(new Media(window));
    }
    
    Document* document() const { return m_window->document(); }

    String type() const;

    bool matchMedium(const String&) const;
    
private:
    Media(DOMWindow*);

    RefPtr<DOMWindow> m_window;
};

} // namespace

#endif // Media_h
