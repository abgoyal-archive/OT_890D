
 
#ifndef WXWEBFRAMEPRIVATE_H
#define WXWEBFRAMEPRIVATE_H

#include "config.h"
#include "EditCommand.h"
#include "EditCommandWx.h"
#include "Frame.h"
#include "wtf/RefPtr.h"
#include "wtf/Vector.h"

class WebFramePrivate {

public:
    WebFramePrivate() :
            frame(0)
    {}

    WTF::Vector<EditCommandWx> undoStack;
    WTF::Vector<EditCommandWx> redoStack;
    
    WTF::RefPtr<WebCore::Frame> frame;
};

#endif
