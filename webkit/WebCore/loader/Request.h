

#ifndef Request_h
#define Request_h

#include <wtf/Vector.h>

namespace WebCore {

    class CachedResource;
    class DocLoader;

    class Request {
    public:
        Request(DocLoader*, CachedResource*, bool incremental, bool skipCanLoadCheck, bool sendResourceLoadCallbacks);
        ~Request();
        
        Vector<char>& buffer() { return m_buffer; }
        CachedResource* cachedResource() { return m_object; }
        DocLoader* docLoader() { return m_docLoader; }

        bool isIncremental() { return m_incremental; }
        void setIsIncremental(bool b = true) { m_incremental = b; }

        bool isMultipart() { return m_multipart; }
        void setIsMultipart(bool b = true) { m_multipart = b; }

        bool shouldSkipCanLoadCheck() const { return m_shouldSkipCanLoadCheck; }
        bool sendResourceLoadCallbacks() const { return m_sendResourceLoadCallbacks; }
        
    private:
        Vector<char> m_buffer;
        CachedResource* m_object;
        DocLoader* m_docLoader;
        bool m_incremental;
        bool m_multipart;
        bool m_shouldSkipCanLoadCheck;
        bool m_sendResourceLoadCallbacks;
    };

} //namespace WebCore

#endif // Request_h
