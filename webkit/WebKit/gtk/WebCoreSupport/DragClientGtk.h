

#ifndef DragClientGtk_h
#define DragClientGtk_h

#include "DragClient.h"

namespace WebKit {
    class DragClient : public WebCore::DragClient {
    public:
        virtual void willPerformDragDestinationAction(WebCore::DragDestinationAction, WebCore::DragData*);
        virtual void willPerformDragSourceAction(WebCore::DragSourceAction, const WebCore::IntPoint&, WebCore::Clipboard*);
        virtual WebCore::DragDestinationAction actionMaskForDrag(WebCore::DragData*);

        virtual WebCore::DragSourceAction dragSourceActionMaskForPoint(const WebCore::IntPoint& windowPoint);

        virtual void startDrag(WebCore::DragImageRef dragImage, const WebCore::IntPoint& dragImageOrigin, const WebCore::IntPoint& eventPos, WebCore::Clipboard*, WebCore::Frame*, bool linkDrag = false);
        virtual WebCore::DragImageRef createDragImageForLink(WebCore::KURL&, const WebCore::String& label, WebCore::Frame*);

        virtual void dragControllerDestroyed();
    };
}

#endif
