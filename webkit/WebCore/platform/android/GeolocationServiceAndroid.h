

#ifndef GeolocationServiceAndroid_h
#define GeolocationServiceAndroid_h

#include "GeolocationService.h"
#include "Timer.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    // The GeolocationServiceBridge is the bridge to the Java implementation of
    // the Geolocation service. It is an implementation detail of
    // GeolocationServiceAndroid.
    class GeolocationServiceBridge;

    class GeolocationServiceAndroid : public GeolocationService {
    public:
        static GeolocationService* create(GeolocationServiceClient*);

        GeolocationServiceAndroid(GeolocationServiceClient*);
        virtual ~GeolocationServiceAndroid() {};

        virtual bool startUpdating(PositionOptions*);
        virtual void stopUpdating();

        virtual Geoposition* lastPosition() const { return m_lastPosition.get(); }
        virtual PositionError* lastError() const { return m_lastError.get(); }

        virtual void suspend();
        virtual void resume();

        // Android-specific
        void newPositionAvailable(PassRefPtr<Geoposition>);
        void newErrorAvailable(PassRefPtr<PositionError>);
        void timerFired(Timer<GeolocationServiceAndroid>* timer);

    private:
        static bool isPositionMovement(Geoposition* position1, Geoposition* position2);
        static bool isPositionMoreAccurate(Geoposition* position1, Geoposition* position2);
        static bool isPositionMoreTimely(Geoposition* position1, Geoposition* position2);

        Timer<GeolocationServiceAndroid> m_timer;
        RefPtr<Geoposition> m_lastPosition;
        RefPtr<PositionError> m_lastError;
        OwnPtr<GeolocationServiceBridge> m_javaBridge;
    };

} // namespace WebCore

#endif // GeolocationServiceAndroid_h
