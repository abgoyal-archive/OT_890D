

#ifndef UserStyleSheetLoader_h
#define UserStyleSheetLoader_h

#include "CachedResourceClient.h"
#include "CachedResourceHandle.h"

#include "Document.h"

namespace WebCore {
    class CachedCSSStyleSheet;
    class String;

    // This class is deprecated and should not be used in any new code. User
    // stylesheet loading should instead happen through Page.
    class UserStyleSheetLoader : CachedResourceClient {
    public:
        UserStyleSheetLoader(PassRefPtr<Document>, const String& url);
        ~UserStyleSheetLoader();

    private:
        virtual void setCSSStyleSheet(const String& URL, const String& charset, const CachedCSSStyleSheet* sheet);

        RefPtr<Document> m_document;
        CachedResourceHandle<CachedCSSStyleSheet> m_cachedSheet;
    };

} // namespace WebCore

#endif // UserStyleSheetLoader_h
