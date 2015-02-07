

#ifndef HTTPHeaderMap_h
#define HTTPHeaderMap_h

#include "AtomicString.h"
#include "AtomicStringHash.h"
#include "StringHash.h"
#include <memory>
#include <utility>
#include <wtf/HashMap.h>
#include <wtf/Vector.h>

namespace WebCore {

    typedef Vector<std::pair<String, String> > CrossThreadHTTPHeaderMapData;

    class HTTPHeaderMap : public HashMap<AtomicString, String, CaseFoldingHash> {
    public:
        // Gets a copy of the data suitable for passing to another thread.
        std::auto_ptr<CrossThreadHTTPHeaderMapData> copyData() const;

        void adopt(std::auto_ptr<CrossThreadHTTPHeaderMapData>);
    };

} // namespace WebCore

#endif // HTTPHeaderMap_h
