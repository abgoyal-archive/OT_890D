

#ifndef SharedWorkerRepository_h
#define SharedWorkerRepository_h

#if ENABLE(SHARED_WORKERS)

#include "ExceptionCode.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

    class KURL;
    class MessagePortChannel;
    class SharedWorker;
    class String;

    // Interface to a repository which manages references to the set of active shared workers.
    class SharedWorkerRepository {
    public:
        // Connects the passed SharedWorker object with the specified worker thread, creating a new thread if necessary.
        static void connect(PassRefPtr<SharedWorker>, PassOwnPtr<MessagePortChannel>, const KURL&, const String& name, ExceptionCode&);
    private:
        SharedWorkerRepository() { }
    };

} // namespace WebCore

#endif // ENABLE(SHARED_WORKERS)

#endif // SharedWorkerRepository_h
