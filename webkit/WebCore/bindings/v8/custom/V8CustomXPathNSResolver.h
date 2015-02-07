


#ifndef V8CustomXPathNSResolver_h
#define V8CustomXPathNSResolver_h

#if ENABLE(XPATH)

#include "XPathNSResolver.h"
#include <v8.h>
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class String;

class V8CustomXPathNSResolver : public XPathNSResolver {
public:
    static PassRefPtr<V8CustomXPathNSResolver> create(v8::Handle<v8::Object> resolver);

    virtual ~V8CustomXPathNSResolver();
    virtual String lookupNamespaceURI(const String& prefix);

private:
    V8CustomXPathNSResolver(v8::Handle<v8::Object> resolver);

    v8::Handle<v8::Object> m_resolver;  // Handle to resolver object.
};

} // namespace WebCore

#endif  // ENABLE(XPATH)

#endif  // V8CustomXPathNSResolver_h
