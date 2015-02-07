

#ifndef PtrAndFlags_h
#define PtrAndFlags_h

#include <wtf/Assertions.h>

namespace WTF {
    template<class T, typename FlagEnum> class PtrAndFlags {
    public:
        PtrAndFlags() : m_ptrAndFlags(0) {}
        PtrAndFlags(T* ptr) : m_ptrAndFlags(0) { set(ptr); }

        bool isFlagSet(FlagEnum flagNumber) const { ASSERT(flagNumber < 2); return m_ptrAndFlags & (1 << flagNumber); }
        void setFlag(FlagEnum flagNumber) { ASSERT(flagNumber < 2); m_ptrAndFlags |= (1 << flagNumber);}
        void clearFlag(FlagEnum flagNumber) { ASSERT(flagNumber < 2); m_ptrAndFlags &= ~(1 << flagNumber);}
        T* get() const { return reinterpret_cast<T*>(m_ptrAndFlags & ~3); }
        void set(T* ptr)
        {
            ASSERT(!(reinterpret_cast<intptr_t>(ptr) & 3));
            m_ptrAndFlags = reinterpret_cast<intptr_t>(ptr) | (m_ptrAndFlags & 3);
#ifndef NDEBUG
            m_leaksPtr = ptr;
#endif
        }

        bool operator!() const { return !get(); }
        T* operator->() const { return reinterpret_cast<T*>(m_ptrAndFlags & ~3); }

    private:
        intptr_t m_ptrAndFlags;
#ifndef NDEBUG
        void* m_leaksPtr; // Only used to allow tools like leaks on OSX to detect that the memory is referenced.
#endif
    };
} // namespace WTF

using WTF::PtrAndFlags;

#endif // PtrAndFlags_h
