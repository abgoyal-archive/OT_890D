

#ifndef History_h
#define History_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class Frame;

    class History : public RefCounted<History> {
    public:
        static PassRefPtr<History> create(Frame* frame) { return adoptRef(new History(frame)); }
        
        Frame* frame() const;
        void disconnectFrame();

        unsigned length() const;
        void back();
        void forward();
        void go(int distance);

    private:
        History(Frame*);

        Frame* m_frame;
    };

} // namespace WebCore

#endif // History_h
