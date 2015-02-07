

#ifndef MarkStack_h
#define MarkStack_h

#include "JSValue.h"

#include <wtf/Noncopyable.h>

namespace JSC {
    class Register;
    
    enum MarkSetProperties { MayContainNullValues, NoNullValues };
    
    class MarkStack : Noncopyable {
    public:
        MarkStack()
            : m_markSets()
            , m_values()
        {
        }

        ALWAYS_INLINE void append(JSValue value)
        {
            ASSERT(value);
            if (value.marked())
                return;
            value.markDirect();
            if (value.hasChildren())
                m_values.append(value.asCell());
        }

        ALWAYS_INLINE void append(JSCell* cell);
        
        ALWAYS_INLINE void appendValues(Register* values, size_t count, MarkSetProperties properties = NoNullValues)
        {
            appendValues(reinterpret_cast<JSValue*>(values), count, properties);
        }

        ALWAYS_INLINE void appendValues(JSValue* values, size_t count, MarkSetProperties properties = NoNullValues)
        {
            if (count)
                m_markSets.append(MarkSet(values, values + count, properties));
        }

        inline void drain();
        void compact();

        ~MarkStack()
        {
            ASSERT(m_markSets.isEmpty());
            ASSERT(m_values.isEmpty());
        }

    private:
        struct MarkSet {
            MarkSet(JSValue* values, JSValue* end, MarkSetProperties properties)
                : m_values(values)
                , m_end(end)
                , m_properties(properties)
            {
            }
            JSValue* m_values;
            JSValue* m_end;
            MarkSetProperties m_properties;
        };

        static void* allocateStack(size_t size);
        static void releaseStack(void* addr, size_t size);

        static void initializePagesize();
        static size_t pageSize()
        {
            if (!s_pageSize)
                initializePagesize();
            return s_pageSize;
        }

        template <typename T> struct MarkStackArray {
            MarkStackArray()
                : m_top(0)
                , m_allocated(MarkStack::pageSize())
                , m_capacity(m_allocated / sizeof(T))
            {
                m_data = reinterpret_cast<T*>(allocateStack(m_allocated));
            }

            ~MarkStackArray()
            {
                releaseStack(m_data, m_allocated);
            }

            void expand()
            {
                size_t oldAllocation = m_allocated;
                m_allocated *= 2;
                m_capacity = m_allocated / sizeof(T);
                void* newData = allocateStack(m_allocated);
                memcpy(newData, m_data, oldAllocation);
                releaseStack(m_data, oldAllocation);
                m_data = reinterpret_cast<T*>(newData);
            }

            inline void append(const T& v)
            {
                if (m_top == m_capacity)
                    expand();
                m_data[m_top++] = v;
            }

            inline T removeLast()
            {
                ASSERT(m_top);
                return m_data[--m_top];
            }

            inline bool isEmpty()
            {
                return m_top == 0;
            }

            inline size_t size() { return m_top; }

            inline void shrinkAllocation(size_t size)
            {
                ASSERT(size <= m_allocated);
                ASSERT(0 == (size % MarkStack::pageSize()));
                if (size == m_allocated)
                    return;
                releaseStack(reinterpret_cast<char*>(m_data) + size, m_allocated - size);
                m_allocated = size;
                m_capacity = m_allocated / sizeof(T);
            }

        private:
            size_t m_top;
            size_t m_allocated;
            size_t m_capacity;
            T* m_data;
        };

        MarkStackArray<MarkSet> m_markSets;
        MarkStackArray<JSCell*> m_values;
        static size_t s_pageSize;
    };
}

#endif
