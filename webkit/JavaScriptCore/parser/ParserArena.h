

#ifndef ParserArena_h
#define ParserArena_h

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace JSC {

    class ParserArenaDeletable;
    class ParserArenaRefCounted;

    class ParserArena {
    public:
        void swap(ParserArena& otherArena)
        {
            m_deletableObjects.swap(otherArena.m_deletableObjects);
            m_refCountedObjects.swap(otherArena.m_refCountedObjects);
        }
        ~ParserArena();

        void deleteWithArena(ParserArenaDeletable* object) { m_deletableObjects.append(object); }
        void derefWithArena(PassRefPtr<ParserArenaRefCounted> object) { m_refCountedObjects.append(object); }

        bool contains(ParserArenaRefCounted*) const;
        ParserArenaRefCounted* last() const;
        void removeLast();

        bool isEmpty() const { return m_deletableObjects.isEmpty() && m_refCountedObjects.isEmpty(); }
        void reset();

    private:
        Vector<ParserArenaDeletable*> m_deletableObjects;
        Vector<RefPtr<ParserArenaRefCounted> > m_refCountedObjects;
    };

}

#endif
