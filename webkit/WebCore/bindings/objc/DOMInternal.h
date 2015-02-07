

// This is lets our internals access DOMObject's _internal field while having
// it be private for clients outside WebKit.
#define private public
#import "DOMObject.h"
#undef private

#import "DOMNodeFilter.h"
#import "DOMXPathNSResolver.h"
#import <wtf/Forward.h>

namespace JSC {
    class JSObject;
    namespace Bindings {
        class RootObject;
    }
}

namespace WebCore {
    class NodeFilter;
#if ENABLE(XPATH)
    class XPathNSResolver;
#endif
}

@interface DOMNodeFilter : DOMObject <DOMNodeFilter>
@end

#if ENABLE(XPATH)
@interface DOMNativeXPathNSResolver : DOMObject <DOMXPathNSResolver>
@end
#endif // ENABLE(XPATH)

// Helper functions for DOM wrappers and gluing to Objective-C

// Create an NSMapTable mapping from pointers to ObjC objects held with zeroing weak references.
NSMapTable* createWrapperCache();
NSMapTable* createWrapperCacheWithIntegerKeys(); // Same, but from integers to ObjC objects.

id createDOMWrapper(JSC::JSObject*, PassRefPtr<JSC::Bindings::RootObject> origin, PassRefPtr<JSC::Bindings::RootObject> current);

NSObject* getDOMWrapper(DOMObjectInternal*);
void addDOMWrapper(NSObject* wrapper, DOMObjectInternal*);
void removeDOMWrapper(DOMObjectInternal*);

template <class Source>
inline id getDOMWrapper(Source impl)
{
    return getDOMWrapper(reinterpret_cast<DOMObjectInternal*>(impl));
}

template <class Source>
inline void addDOMWrapper(NSObject* wrapper, Source impl)
{
    addDOMWrapper(wrapper, reinterpret_cast<DOMObjectInternal*>(impl));
}

DOMNodeFilter *kit(WebCore::NodeFilter*);
WebCore::NodeFilter* core(DOMNodeFilter *);

#if ENABLE(XPATH)
DOMNativeXPathNSResolver *kit(WebCore::XPathNSResolver*);
WebCore::XPathNSResolver* core(DOMNativeXPathNSResolver *);
#endif // ENABLE(XPATH)
