

#ifndef ResourceRequestBase_h
#define ResourceRequestBase_h

#include "FormData.h"
#include "KURL.h"
#include "HTTPHeaderMap.h"

#include <memory>
#include <wtf/OwnPtr.h>

namespace WebCore {

    enum ResourceRequestCachePolicy {
        UseProtocolCachePolicy, // normal load
        ReloadIgnoringCacheData, // reload
        ReturnCacheDataElseLoad, // back/forward or encoding change - allow stale data
        ReturnCacheDataDontLoad, // results of a post - allow stale data and only use cache
    };

    const int unspecifiedTimeoutInterval = INT_MAX;

    struct ResourceRequest;
    struct CrossThreadResourceRequestData;

    // Do not use this type directly.  Use ResourceRequest instead.
    class ResourceRequestBase {
    public:
        static std::auto_ptr<ResourceRequest> adopt(std::auto_ptr<CrossThreadResourceRequestData>);

        // Gets a copy of the data suitable for passing to another thread.
        std::auto_ptr<CrossThreadResourceRequestData> copyData() const;

        bool isNull() const;
        bool isEmpty() const;

        const KURL& url() const;
        void setURL(const KURL& url);

        void removeCredentials();

        ResourceRequestCachePolicy cachePolicy() const;
        void setCachePolicy(ResourceRequestCachePolicy cachePolicy);
        
        double timeoutInterval() const;
        void setTimeoutInterval(double timeoutInterval);
        
        const KURL& firstPartyForCookies() const;
        void setFirstPartyForCookies(const KURL& firstPartyForCookies);
        
        const String& httpMethod() const;
        void setHTTPMethod(const String& httpMethod);
        
        const HTTPHeaderMap& httpHeaderFields() const;
        String httpHeaderField(const AtomicString& name) const;
        void setHTTPHeaderField(const AtomicString& name, const String& value);
        void addHTTPHeaderField(const AtomicString& name, const String& value);
        void addHTTPHeaderFields(const HTTPHeaderMap& headerFields);
        
        String httpContentType() const { return httpHeaderField("Content-Type");  }
        void setHTTPContentType(const String& httpContentType) { setHTTPHeaderField("Content-Type", httpContentType); }
        
        String httpReferrer() const { return httpHeaderField("Referer"); }
        void setHTTPReferrer(const String& httpReferrer) { setHTTPHeaderField("Referer", httpReferrer); }
        void clearHTTPReferrer() { m_httpHeaderFields.remove("Referer"); }
        
        String httpOrigin() const { return httpHeaderField("Origin"); }
        void setHTTPOrigin(const String& httpOrigin) { setHTTPHeaderField("Origin", httpOrigin); }
        void clearHTTPOrigin() { m_httpHeaderFields.remove("Origin"); }

        String httpUserAgent() const { return httpHeaderField("User-Agent"); }
        void setHTTPUserAgent(const String& httpUserAgent) { setHTTPHeaderField("User-Agent", httpUserAgent); }

        String httpAccept() const { return httpHeaderField("Accept"); }
        void setHTTPAccept(const String& httpAccept) { setHTTPHeaderField("Accept", httpAccept); }

        void setResponseContentDispositionEncodingFallbackArray(const String& encoding1, const String& encoding2 = String(), const String& encoding3 = String());

        FormData* httpBody() const;
        void setHTTPBody(PassRefPtr<FormData> httpBody);
        
        bool allowHTTPCookies() const;
        void setAllowHTTPCookies(bool allowHTTPCookies);

        bool isConditional() const;

        // Whether the associated ResourceHandleClient needs to be notified of
        // upload progress made for that resource.
        bool reportUploadProgress() const { return m_reportUploadProgress; }
        void setReportUploadProgress(bool reportUploadProgress) { m_reportUploadProgress = reportUploadProgress; }

    protected:
        // Used when ResourceRequest is initialized from a platform representation of the request
        ResourceRequestBase()
            : m_resourceRequestUpdated(false)
            , m_platformRequestUpdated(true)
            , m_reportUploadProgress(false)
        {
        }

        ResourceRequestBase(const KURL& url, ResourceRequestCachePolicy policy)
            : m_url(url)
            , m_cachePolicy(policy)
            , m_timeoutInterval(unspecifiedTimeoutInterval)
            , m_httpMethod("GET")
            , m_allowHTTPCookies(true)
            , m_resourceRequestUpdated(true)
            , m_platformRequestUpdated(false)
            , m_reportUploadProgress(false)
        {
        }

        void updatePlatformRequest() const; 
        void updateResourceRequest() const; 

        KURL m_url;

        ResourceRequestCachePolicy m_cachePolicy;
        double m_timeoutInterval;
        KURL m_firstPartyForCookies;
        String m_httpMethod;
        HTTPHeaderMap m_httpHeaderFields;
        Vector<String> m_responseContentDispositionEncodingFallbackArray;
        RefPtr<FormData> m_httpBody;
        bool m_allowHTTPCookies;
        mutable bool m_resourceRequestUpdated;
        mutable bool m_platformRequestUpdated;
        bool m_reportUploadProgress;

    private:
        const ResourceRequest& asResourceRequest() const;
    };

    bool equalIgnoringHeaderFields(const ResourceRequestBase&, const ResourceRequestBase&);

    bool operator==(const ResourceRequestBase&, const ResourceRequestBase&);
    inline bool operator!=(ResourceRequestBase& a, const ResourceRequestBase& b) { return !(a == b); }

    struct CrossThreadResourceRequestData {
        KURL m_url;

        ResourceRequestCachePolicy m_cachePolicy;
        double m_timeoutInterval;
        KURL m_firstPartyForCookies;

        String m_httpMethod;
        OwnPtr<CrossThreadHTTPHeaderMapData> m_httpHeaders;
        Vector<String> m_responseContentDispositionEncodingFallbackArray;
        RefPtr<FormData> m_httpBody;
        bool m_allowHTTPCookies;
    };
    
    unsigned initializeMaximumHTTPConnectionCountPerHost();

} // namespace WebCore

#endif // ResourceRequestBase_h
