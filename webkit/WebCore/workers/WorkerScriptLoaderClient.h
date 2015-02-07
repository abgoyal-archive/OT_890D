

#ifndef WorkerScriptLoaderClient_h
#define WorkerScriptLoaderClient_h

#if ENABLE(WORKERS)

namespace WebCore {

    class WorkerScriptLoaderClient {
    public:
        virtual void notifyFinished() { }
        
    protected:
        virtual ~WorkerScriptLoaderClient() { }
    };

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // WorkerScriptLoaderClient_h
