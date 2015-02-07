

#ifndef ResourceRequest_h
#define ResourceRequest_h

#include "CString.h"
#include "ResourceRequestBase.h"

namespace WebCore {

    class Frame;

    struct ResourceRequest : public ResourceRequestBase {
    public:
        enum TargetType {
            TargetIsMainFrame,
            TargetIsSubFrame,
            TargetIsSubResource,
            TargetIsObject,
            TargetIsMedia
        };

        ResourceRequest(const String& url) 
            : ResourceRequestBase(KURL(url), UseProtocolCachePolicy)
            , m_requestorID(0)
            , m_requestorProcessID(0)
            , m_appCacheContextID(0)
            , m_targetType(TargetIsSubResource)
        {
        }

        ResourceRequest(const KURL& url, const CString& securityInfo) 
            : ResourceRequestBase(url, UseProtocolCachePolicy)
            , m_requestorID(0)
            , m_requestorProcessID(0)
            , m_appCacheContextID(0)
            , m_targetType(TargetIsSubResource)
            , m_securityInfo(securityInfo)
        {
        }

        ResourceRequest(const KURL& url) 
            : ResourceRequestBase(url, UseProtocolCachePolicy)
            , m_requestorID(0)
            , m_requestorProcessID(0)
            , m_appCacheContextID(0)
            , m_targetType(TargetIsSubResource)
        {
        }

        ResourceRequest(const KURL& url, const String& referrer, ResourceRequestCachePolicy policy = UseProtocolCachePolicy) 
            : ResourceRequestBase(url, policy)
            , m_requestorID(0)
            , m_requestorProcessID(0)
            , m_appCacheContextID(0)
            , m_targetType(TargetIsSubResource)
        {
            setHTTPReferrer(referrer);
        }
        
        ResourceRequest()
            : ResourceRequestBase(KURL(), UseProtocolCachePolicy)
            , m_requestorID(0)
            , m_requestorProcessID(0)
            , m_appCacheContextID(0)
            , m_targetType(TargetIsSubResource)
        {
        }

        // Allows the request to be matched up with its requestor.
        int requestorID() const { return m_requestorID; }
        void setRequestorID(int requestorID) { m_requestorID = requestorID; }

        // What this request is for.
        TargetType targetType() const { return m_targetType; }
        void setTargetType(TargetType type) { m_targetType = type; }

        // The process id of the process from which this request originated. In
        // the case of out-of-process plugins, this allows to link back the
        // request to the plugin process (as it is processed through a render
        // view process).
        int requestorProcessID() const { return m_requestorProcessID; }
        void setRequestorProcessID(int requestorProcessID) { m_requestorProcessID = requestorProcessID; }

        // Allows the request to be matched up with its app cache context.
        int appCacheContextID() const { return m_appCacheContextID; }
        void setAppCacheContextID(int id) { m_appCacheContextID = id; }

        // Opaque buffer that describes the security state (including SSL
        // connection state) for the resource that should be reported when the
        // resource has been loaded.  This is used to simulate secure
        // connection for request (typically when showing error page, so the
        // error page has the errors of the page that actually failed).  Empty
        // string if not a secure connection.
        CString securityInfo() const { return m_securityInfo; }
        void setSecurityInfo(const CString& value) { m_securityInfo = value; }

    private:
        friend class ResourceRequestBase;

        void doUpdatePlatformRequest() {}
        void doUpdateResourceRequest() {}

        int m_requestorID;
        int m_requestorProcessID;
        int m_appCacheContextID;
        TargetType m_targetType;
        CString m_securityInfo;
    };

} // namespace WebCore

#endif
