

#ifndef EvalCodeCache_h
#define EvalCodeCache_h

#include "JSGlobalObject.h"
#include "Nodes.h"
#include "Parser.h"
#include "SourceCode.h"
#include "UString.h"
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace JSC {

    class EvalCodeCache {
    public:
        PassRefPtr<EvalNode> get(ExecState* exec, const UString& evalSource, ScopeChainNode* scopeChain, JSValue& exceptionValue)
        {
            RefPtr<EvalNode> evalNode;

            if (evalSource.size() < maxCacheableSourceLength && (*scopeChain->begin())->isVariableObject())
                evalNode = m_cacheMap.get(evalSource.rep());

            if (!evalNode) {
                int errorLine;
                UString errorMessage;
                
                SourceCode source = makeSource(evalSource);
                evalNode = exec->globalData().parser->parse<EvalNode>(exec, exec->dynamicGlobalObject()->debugger(), source, &errorLine, &errorMessage);
                if (evalNode) {
                    if (evalSource.size() < maxCacheableSourceLength && (*scopeChain->begin())->isVariableObject() && m_cacheMap.size() < maxCacheEntries)
                        m_cacheMap.set(evalSource.rep(), evalNode);
                } else {
                    exceptionValue = Error::create(exec, SyntaxError, errorMessage, errorLine, source.provider()->asID(), 0);
                    return 0;
                }
            }

            return evalNode.release();
        }

        bool isEmpty() const { return m_cacheMap.isEmpty(); }

        void markAggregate(MarkStack& markStack)
        {
            EvalCacheMap::iterator end = m_cacheMap.end();
            for (EvalCacheMap::iterator ptr = m_cacheMap.begin(); ptr != end; ++ptr)
                ptr->second->markAggregate(markStack);
        }
    private:
        static const int maxCacheableSourceLength = 256;
        static const int maxCacheEntries = 64;

        typedef HashMap<RefPtr<UString::Rep>, RefPtr<EvalNode> > EvalCacheMap;
        EvalCacheMap m_cacheMap;
    };

} // namespace JSC

#endif // EvalCodeCache_h
