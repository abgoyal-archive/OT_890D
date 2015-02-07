

#ifndef V8EventListenerList_h
#define V8EventListenerList_h

#include <v8.h>
#include <wtf/Vector.h>
#include <wtf/HashMap.h>

#include "PassRefPtr.h"

namespace WebCore {
    class Frame;
    class V8EventListener;
    class V8EventListenerListIterator;

    // This is a container for V8EventListener objects that uses the identity hash of the v8::Object to
    // speed up lookups
    class V8EventListenerList {
    public:
        // Because v8::Object identity hashes are not guaranteed to be unique, we unfortunately can't just map
        // an int to V8EventListener. Instead we define a HashMap of int to Vector of V8EventListener
        // called a ListenerMultiMap.
        typedef Vector<V8EventListener*>* Values;
        struct ValuesTraits : HashTraits<Values> {
            static const bool needsDestruction = true;
        };
        typedef HashMap<int, Values, DefaultHash<int>::Hash, HashTraits<int>, ValuesTraits> ListenerMultiMap;

        V8EventListenerList();
        ~V8EventListenerList();

        friend class V8EventListenerListIterator;
        typedef V8EventListenerListIterator iterator;        

        iterator begin();
        iterator end();

        void add(V8EventListener*);
        void remove(V8EventListener*);
        V8EventListener* find(v8::Local<v8::Object>, bool isAttribute);
        void clear();
        size_t size() { return m_table.size(); }

        PassRefPtr<V8EventListener> findWrapper(v8::Local<v8::Value>, bool isAttribute);
        template<typename WrapperType>
        PassRefPtr<V8EventListener> findOrCreateWrapper(Frame*, v8::Local<v8::Value>, bool isAttribute);

    private:
        ListenerMultiMap m_table;

        // we also keep a reverse mapping of V8EventListener to v8::Object identity hash,
        // in order to speed up removal by V8EventListener
        HashMap<V8EventListener*, int> m_reverseTable;
    };

    class V8EventListenerListIterator {
    public:
        ~V8EventListenerListIterator();
        void operator++();
        bool operator==(const V8EventListenerListIterator&);
        bool operator!=(const V8EventListenerListIterator&);
        V8EventListener* operator*();
    private:
        friend class V8EventListenerList;
        explicit V8EventListenerListIterator(V8EventListenerList*);
        V8EventListenerListIterator(V8EventListenerList*, bool shouldSeekToEnd);
        void seekToEnd();

        V8EventListenerList* m_list;
        V8EventListenerList::ListenerMultiMap::iterator m_iter;
        size_t m_vectorIndex;
    };

    template<typename WrapperType>
    PassRefPtr<V8EventListener> V8EventListenerList::findOrCreateWrapper(Frame* frame, v8::Local<v8::Value> object, bool isAttribute)
    {
        ASSERT(v8::Context::InContext());
        if (!object->IsObject())
            return 0;

        // FIXME: Should this be v8::Local<v8::Object>::Cast instead?
        V8EventListener* wrapper = find(object->ToObject(), isAttribute);
        if (wrapper)
            return wrapper;

        // Create a new one, and add to cache.
        RefPtr<WrapperType> newListener = WrapperType::create(frame, v8::Local<v8::Object>::Cast(object), isAttribute);
        add(newListener.get());

        return newListener;
    };

} // namespace WebCore

#endif // V8EventListenerList_h
