

#ifndef OwnHandle_h
#define OwnHandle_h

#include <v8.h>

namespace WebCore {

    template<typename T>
    class OwnHandle {
    public:
        OwnHandle() { }
        explicit OwnHandle(v8::Handle<T> handle) : m_handle(v8::Persistent<T>::New(handle)) { }
        ~OwnHandle() { clear(); }

        v8::Handle<T> get() const { return m_handle; }
        void set(v8::Handle<T> handle) { clear(); m_handle = v8::Persistent<T>::New(handle); }

        // FIXME: What if we release a weak handle?  Won't the callback do the wrong thing?
        v8::Persistent<T> release() { v8::Persistent<T> result = m_handle; m_handle.Clear(); return result; }
        void adopt(v8::Persistent<T> handle) { clear(); m_handle = handle; }

        // Note: This is clear in the OwnPtr sense, not the v8::Handle sense.
        void clear()
        {
            if (m_handle.IsEmpty())
                return;
            m_handle.Dispose();
            m_handle.Clear();
        }

        // Make the underlying handle weak.  The client doesn't get a callback,
        // we just make the handle empty.
        void makeWeak()
        {
            if (m_handle.IsEmpty())
                return;
            m_handle.MakeWeak(this, &OwnHandle<T>::weakCallback);
        }

    private:
        static void weakCallback(v8::Persistent<v8::Value> object, void* ownHandle)
        {
            OwnHandle<T>* handle = static_cast<OwnHandle<T>*>(ownHandle);
            handle->clear();
        }

        v8::Persistent<T> m_handle;
    };

} // namespace WebCore

#endif // OwnHandle_h
