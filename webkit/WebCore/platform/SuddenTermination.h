

#ifndef SuddenTermination_h
#define SuddenTermination_h

#include <wtf/Platform.h>

namespace WebCore {

    void disableSuddenTermination();
    void enableSuddenTermination();

#if (!PLATFORM(MAC) || defined(BUILDING_ON_TIGER) || defined(BUILDING_ON_LEOPARD)) && !PLATFORM(CHROMIUM)
    inline void disableSuddenTermination() { }
    inline void enableSuddenTermination() { }
#endif

} // namespace WebCore

#endif // SuddenTermination_h
