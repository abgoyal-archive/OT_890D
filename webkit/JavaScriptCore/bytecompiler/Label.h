

#ifndef Label_h
#define Label_h

#include "CodeBlock.h"
#include "Instruction.h"
#include <wtf/Assertions.h>
#include <wtf/Vector.h>
#include <limits.h>

namespace JSC {

    class Label {
    public:
        explicit Label(CodeBlock* codeBlock)
            : m_refCount(0)
            , m_location(invalidLocation)
            , m_codeBlock(codeBlock)
        {
        }

        void setLocation(unsigned location)
        {
            m_location = location;

            unsigned size = m_unresolvedJumps.size();
            for (unsigned i = 0; i < size; ++i) {
                unsigned j = m_unresolvedJumps[i];
                m_codeBlock->instructions()[j].u.operand = m_location - j;
            }
        }

        int offsetFrom(int location) const
        {
            if (m_location == invalidLocation) {
                m_unresolvedJumps.append(location);
                return 0;
            }
            return m_location - location;
        }

        void ref() { ++m_refCount; }
        void deref()
        {
            --m_refCount;
            ASSERT(m_refCount >= 0);
        }
        int refCount() const { return m_refCount; }

        bool isForward() const { return m_location == invalidLocation; }

    private:
        typedef Vector<int, 8> JumpVector;

        static const unsigned invalidLocation = UINT_MAX;

        int m_refCount;
        unsigned m_location;
        CodeBlock* m_codeBlock;
        mutable JumpVector m_unresolvedJumps;
    };

} // namespace JSC

#endif // Label_h
