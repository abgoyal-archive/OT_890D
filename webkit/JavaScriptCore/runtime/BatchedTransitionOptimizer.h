
// -*- mode: c++; c-basic-offset: 4 -*-

#ifndef BatchedTransitionOptimizer_h
#define BatchedTransitionOptimizer_h

#include <wtf/Noncopyable.h>
#include "JSObject.h"

namespace JSC {

    class BatchedTransitionOptimizer : public Noncopyable {
    public:
        BatchedTransitionOptimizer(JSObject* object)
            : m_object(object)
        {
            if (!m_object->structure()->isDictionary())
                m_object->setStructure(Structure::toDictionaryTransition(m_object->structure()));
        }

        ~BatchedTransitionOptimizer()
        {
            m_object->setStructure(Structure::fromDictionaryTransition(m_object->structure()));
        }

    private:
        JSObject* m_object;
    };

} // namespace JSC

#endif // BatchedTransitionOptimizer_h
