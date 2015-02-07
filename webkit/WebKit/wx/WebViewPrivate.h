
 
#ifndef WXWEBVIEWPRIVATE_H
#define WXWEBVIEWPRIVATE_H

#include "config.h"
#include "HTMLFrameOwnerElement.h"
#include "Page.h"
#include "wtf/RefPtr.h"
#include "KURL.h"

#include <wx/timer.h>

class WebViewPrivate 
{
public:
    WebViewPrivate() :
        page(0)
    {}
    
    WebCore::Page* page;

    wxTimer tripleClickTimer;
    wxPoint tripleClickPos;
};

class WebViewFrameData
{
public:
    WebCore::KURL url;
    WebCore::String name;
    WebCore::HTMLFrameOwnerElement* ownerElement;
    
    WebCore::String referrer;
    bool allowsScrolling;
    int marginWidth;
    int marginHeight;    
};

#endif
