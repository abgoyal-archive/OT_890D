

#ifndef PageGroup_h
#define PageGroup_h

#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include "LinkHash.h"
#include "StringHash.h"

namespace WebCore {

    class KURL;
    class Page;
    class StorageNamespace;

    class PageGroup : public Noncopyable {
    public:
        PageGroup(const String& name);
        PageGroup(Page*);

        static PageGroup* pageGroup(const String& groupName);
        static void closeLocalStorage();

        const HashSet<Page*>& pages() const { return m_pages; }

        void addPage(Page*);
        void removePage(Page*);

        bool isLinkVisited(LinkHash);

        void addVisitedLink(const KURL&);
        void addVisitedLink(const UChar*, size_t);
        void removeVisitedLinks();

        static void setShouldTrackVisitedLinks(bool);
        static void removeAllVisitedLinks();

        const String& name() { return m_name; }
        unsigned identifier() { return m_identifier; }

#if ENABLE(DOM_STORAGE)
        StorageNamespace* localStorage();
#endif

    private:
        void addVisitedLink(LinkHash stringHash);
#if ENABLE(DOM_STORAGE)
        bool hasLocalStorage() { return m_localStorage; }
#endif
        String m_name;

        HashSet<Page*> m_pages;

        HashSet<LinkHash, LinkHashHash> m_visitedLinkHashes;
        bool m_visitedLinksPopulated;

        unsigned m_identifier;
#if ENABLE(DOM_STORAGE)
        RefPtr<StorageNamespace> m_localStorage;
#endif
    };

} // namespace WebCore
    
#endif // PageGroup_h
