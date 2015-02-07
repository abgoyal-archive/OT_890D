

#ifndef MediaDocument_h
#define MediaDocument_h

#if ENABLE(VIDEO)

#include "HTMLDocument.h"

namespace WebCore {

class MediaDocument : public HTMLDocument {
public:
    static PassRefPtr<MediaDocument> create(Frame* frame)
    {
        return new MediaDocument(frame);
    }

    virtual void defaultEventHandler(Event*);

    void mediaElementSawUnsupportedTracks();

private:
    MediaDocument(Frame*);
    virtual ~MediaDocument();
    Timer<MediaDocument> m_replaceMediaElementTimer;

    virtual bool isMediaDocument() const { return true; }        
    virtual Tokenizer* createTokenizer();

    void replaceMediaElementTimerFired(Timer<MediaDocument>*);
};
    
}

#endif
#endif
