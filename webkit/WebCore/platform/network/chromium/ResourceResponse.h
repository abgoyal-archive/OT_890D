

#ifndef ResourceResponse_h
#define ResourceResponse_h

#include "CString.h"
#include "NotImplemented.h"
#include "ResourceResponseBase.h"

namespace WebCore {

    class ResourceResponse : public ResourceResponseBase {
    public:
        ResourceResponse()
            : m_isContentFiltered(false)
            , m_appCacheID(0)
        {
        }

        ResourceResponse(const KURL& url, const String& mimeType, long long expectedLength, const String& textEncodingName, const String& filename)
            : ResourceResponseBase(url, mimeType, expectedLength, textEncodingName, filename)
            , m_isContentFiltered(false)
            , m_appCacheID(0)
        {
        }

        const CString& getSecurityInfo() const { return m_securityInfo; }
        void setSecurityInfo(const CString& securityInfo)
        {
            m_securityInfo = securityInfo;
        }

        bool isContentFiltered() const { return m_isContentFiltered; }
        void setIsContentFiltered(bool isContentFiltered)
        {
            m_isContentFiltered = isContentFiltered;
        }

        long long getAppCacheID() const { return m_appCacheID; }
        void setAppCacheID(long long id)
        {
            m_appCacheID = id;
        }

    private:
        friend class ResourceResponseBase;

        // An opaque value that contains some information regarding the security of
        // the connection for this request, such as SSL connection info (empty
        // string if not over HTTPS).
        CString m_securityInfo;

        void doUpdateResourceResponse()
        {
            notImplemented();
        }

        // Whether the contents for this response has been altered/blocked (usually
        // for security reasons.
        bool m_isContentFiltered;

        // The id of the appcache this response was retrieved from, or zero if
        // the response was not retrieved from an appcache.
        long long m_appCacheID;
    };

} // namespace WebCore

#endif
