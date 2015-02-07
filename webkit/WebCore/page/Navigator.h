

#ifndef Navigator_h
#define Navigator_h

#include "NavigatorBase.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class Frame;
    class Geolocation;
    class MimeTypeArray;
    class PluginData;
    class PluginArray;
    class String;

    class Navigator : public NavigatorBase, public RefCounted<Navigator> {
    public:
        static PassRefPtr<Navigator> create(Frame* frame) { return adoptRef(new Navigator(frame)); }
        ~Navigator();

        void disconnectFrame();
        Frame* frame() const { return m_frame; }

        String appVersion() const;
        String language() const;
        PluginArray* plugins() const;
        MimeTypeArray* mimeTypes() const;
        bool cookieEnabled() const;
        bool javaEnabled() const;

        virtual String userAgent() const;

        Geolocation* geolocation() const;
        // This is used for GC marking.
        Geolocation* optionalGeolocation() const { return m_geolocation.get(); }

    private:
        Navigator(Frame*);
        Frame* m_frame;
        mutable RefPtr<PluginArray> m_plugins;
        mutable RefPtr<MimeTypeArray> m_mimeTypes;
        mutable RefPtr<Geolocation> m_geolocation;
    };

}

#endif
