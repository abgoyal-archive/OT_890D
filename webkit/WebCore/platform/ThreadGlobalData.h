

#ifndef ThreadGlobalData_h
#define ThreadGlobalData_h

#include "StringHash.h"
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

    class EventNames;
    struct ICUConverterWrapper;
    struct TECConverterWrapper;
    class ThreadTimers;

    class ThreadGlobalData : public Noncopyable {
    public:
        ThreadGlobalData();
        ~ThreadGlobalData();

        EventNames& eventNames() { return *m_eventNames; }
        StringImpl* emptyString() { return m_emptyString; }
        HashSet<StringImpl*>& atomicStringTable() { return *m_atomicStringTable; }
        ThreadTimers& threadTimers() { return *m_threadTimers; }

#if USE(ICU_UNICODE) || USE(GLIB_ICU_UNICODE_HYBRID)
        ICUConverterWrapper& cachedConverterICU() { return *m_cachedConverterICU; }
#endif

#if PLATFORM(MAC)
        TECConverterWrapper& cachedConverterTEC() { return *m_cachedConverterTEC; }
#endif

    private:
        StringImpl* m_emptyString;
        HashSet<StringImpl*>* m_atomicStringTable;
        EventNames* m_eventNames;
        ThreadTimers* m_threadTimers;

#ifndef NDEBUG
        bool m_isMainThread;
#endif

#if USE(ICU_UNICODE) || USE(GLIB_ICU_UNICODE_HYBRID)
        ICUConverterWrapper* m_cachedConverterICU;
#endif

#if PLATFORM(MAC)
        TECConverterWrapper* m_cachedConverterTEC;
#endif
    };

    ThreadGlobalData& threadGlobalData();

} // namespace WebCore

#endif // ThreadGlobalData_h
