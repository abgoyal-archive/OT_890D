

#ifndef DefaultSharedWorkerRepository_h
#define DefaultSharedWorkerRepository_h

#if ENABLE(SHARED_WORKERS)

#include "SharedWorkerRepository.h"
#include "StringHash.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

    class KURL;
    class ScriptExecutionContext;
    class SecurityOrigin;
    class SharedWorkerProxy;

    struct SecurityOriginHash;
    struct SecurityOriginTraits;

    // Platform-specific implementation of the SharedWorkerRepository static interface.
    class DefaultSharedWorkerRepository : public Noncopyable {
    public:
        // Invoked once the worker script has been loaded to fire up the worker thread.
        void workerScriptLoaded(SharedWorkerProxy&, const String& userAgent, const String& workerScript, PassOwnPtr<MessagePortChannel>);

        // Internal implementation of SharedWorkerRepository::connect()
        void connectToWorker(PassRefPtr<SharedWorker>, PassOwnPtr<MessagePortChannel>, const KURL&, const String& name, ExceptionCode&);

        static DefaultSharedWorkerRepository& instance();
    private:
        DefaultSharedWorkerRepository();
        ~DefaultSharedWorkerRepository();

        PassRefPtr<SharedWorkerProxy> getProxy(const String& name, const KURL&);
        // Mutex used to protect internal data structures.
        Mutex m_lock;

        typedef HashMap<String, RefPtr<SharedWorkerProxy> > SharedWorkerNameMap;
        typedef HashMap<RefPtr<SecurityOrigin>, SharedWorkerNameMap*, SecurityOriginHash> SharedWorkerProxyCache;

        // Items in this cache may be freed on another thread, so all keys and values must be either copied before insertion or thread safe.
        SharedWorkerProxyCache m_cache;
    };

} // namespace WebCore

#endif // ENABLE(SHARED_WORKERS)

#endif // DefaultSharedWorkerRepository_h
