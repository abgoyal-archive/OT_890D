

#ifndef PlatformTouchEvent_h
#define PlatformTouchEvent_h

#if ENABLE(TOUCH_EVENTS) // Android

#include "IntPoint.h"

namespace WebCore {

    enum TouchEventType {TouchEventStart, TouchEventMove, TouchEventEnd, TouchEventCancel};

    class PlatformTouchEvent {
    public:
        PlatformTouchEvent()
            : m_eventType(TouchEventCancel)
        {
        }

        PlatformTouchEvent(const IntPoint& pos, const IntPoint& globalPos, TouchEventType eventType)
            : m_position(pos)
            , m_globalPosition(globalPos)
            , m_eventType(eventType)
        {
        }

        const IntPoint& pos() const { return m_position; }
        int x() const { return m_position.x(); }
        int y() const { return m_position.y(); }
        int globalX() const { return m_globalPosition.x(); }
        int globalY() const { return m_globalPosition.y(); }
        TouchEventType eventType() const { return m_eventType; }

    private:
        IntPoint m_position;
        IntPoint m_globalPosition;
        TouchEventType m_eventType;
    };

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)

#endif // PlatformTouchEvent_h
