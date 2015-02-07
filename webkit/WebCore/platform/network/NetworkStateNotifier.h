

#ifndef NetworkStateNotifier_h
#define NetworkStateNotifier_h

#if PLATFORM(MAC)

#include <wtf/RetainPtr.h>
#include "Timer.h"

typedef const struct __CFArray * CFArrayRef;
typedef const struct __SCDynamicStore * SCDynamicStoreRef;

#elif PLATFORM(CHROMIUM)

#include "NetworkStateNotifierPrivate.h"

#elif PLATFORM(WIN)

#include <windows.h>

#endif

namespace WebCore {

class NetworkStateNotifier {
public:
    NetworkStateNotifier();
    void setNetworkStateChangedFunction(void (*)());
    
    bool onLine() const { return m_isOnLine; }
    
private:    
    bool m_isOnLine;
    void (*m_networkStateChangedFunction)();

    void updateState();

#if PLATFORM(MAC)
    void networkStateChangeTimerFired(Timer<NetworkStateNotifier>*);

    static void dynamicStoreCallback(SCDynamicStoreRef, CFArrayRef changedKeys, void *info); 

    RetainPtr<SCDynamicStoreRef> m_store;
    Timer<NetworkStateNotifier> m_networkStateChangeTimer;

#elif PLATFORM(WIN)
    static void CALLBACK addrChangeCallback(void*, BOOLEAN timedOut);
    static void callAddressChanged(void*);
    void addressChanged();
    
    void registerForAddressChange();
    HANDLE m_waitHandle;
    OVERLAPPED m_overlapped;

#elif PLATFORM(ANDROID)
public:
	void networkStateChange(bool online);

#elif PLATFORM(CHROMIUM)
    NetworkStateNotifierPrivate p;
#endif
};

#if !PLATFORM(MAC) && !PLATFORM(WIN) && !PLATFORM(CHROMIUM)

inline NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(true)
    , m_networkStateChangedFunction(0)
{    
}

inline void NetworkStateNotifier::updateState() { }

#endif

NetworkStateNotifier& networkStateNotifier();
    
};

#endif // NetworkStateNotifier_h
