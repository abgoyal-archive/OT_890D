

#ifndef InspectorResource_h
#define InspectorResource_h

#include "HTTPHeaderMap.h"
#include "KURL.h"
#include "ScriptObject.h"
#include "ScriptState.h"
#include "ScriptString.h"

#include <wtf/CurrentTime.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class CachedResource;
    class DocumentLoader;
    class InspectorFrontend;
    class Frame;
    class ResourceResponse;

    struct ResourceRequest;

    class InspectorResource : public RefCounted<InspectorResource> {
    public:

        // Keep these in sync with WebInspector.Resource.Type
        enum Type {
            Doc,
            Stylesheet,
            Image,
            Font,
            Script,
            XHR,
            Media,
            Other
        };

        static PassRefPtr<InspectorResource> create(long long identifier, DocumentLoader* loader)
        {
            return adoptRef(new InspectorResource(identifier, loader));
        }

        static PassRefPtr<InspectorResource> createCached(long long identifier, DocumentLoader*, const CachedResource*);

        ~InspectorResource();

        void createScriptObject(InspectorFrontend* frontend);
        void updateScriptObject(InspectorFrontend* frontend);
        void releaseScriptObject(InspectorFrontend* frontend, bool callRemoveResource);

        void updateRequest(const ResourceRequest&);
        void updateResponse(const ResourceResponse&);

        void setXMLHttpResponseText(const ScriptString& data);

        String sourceString() const;
        bool isSameLoader(DocumentLoader* loader) const { return loader == m_loader; }
        void markMainResource() { m_isMainResource = true; }
        long long identifier() const { return m_identifier; }
        String requestURL() const { return m_requestURL.string(); }
        Frame* frame() const { return m_frame.get(); }
        const String& mimeType() const { return m_mimeType; }
        void startTiming();
        void markResponseReceivedTime();
        void endTiming();

        void markFailed();
        void addLength(int lengthReceived);

    private:
        enum ChangeType {
            NoChange = 0,
            RequestChange = 1,
            ResponseChange = 2,
            TypeChange = 4,
            LengthChange = 8,
            CompletionChange = 16,
            TimingChange = 32
        };

        class Changes {
        public:
            Changes() : m_change(NoChange) {}

            inline bool hasChange(ChangeType change) { return (m_change & change) || !(m_change + change); }
            inline void set(ChangeType change)
            {
                m_change = static_cast<ChangeType>(static_cast<unsigned>(m_change) | static_cast<unsigned>(change));            
            }
            inline void clear(ChangeType change)
            {
                m_change = static_cast<ChangeType>(static_cast<unsigned>(m_change) & ~static_cast<unsigned>(change));
            }

            inline void setAll() { m_change = static_cast<ChangeType>(63); }
            inline void clearAll() { m_change = NoChange; }

        private:
            ChangeType m_change;
        };

        InspectorResource(long long identifier, DocumentLoader*);
        Type type() const;

        long long m_identifier;
        RefPtr<DocumentLoader> m_loader;
        RefPtr<Frame> m_frame;
        KURL m_requestURL;
        HTTPHeaderMap m_requestHeaderFields;
        HTTPHeaderMap m_responseHeaderFields;
        String m_mimeType;
        String m_suggestedFilename;
        bool m_scriptObjectCreated;
        long long m_expectedContentLength;
        bool m_cached;
        bool m_finished;
        bool m_failed;
        int m_length;
        int m_responseStatusCode;
        double m_startTime;
        double m_responseReceivedTime;
        double m_endTime;
        ScriptString m_xmlHttpResponseText;
        Changes m_changes;
        bool m_isMainResource;
    };

} // namespace WebCore

#endif // InspectorResource_h
