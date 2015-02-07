

#ifndef AccessibilityObjectWrapper_h
#define AccessibilityObjectWrapper_h

namespace WebCore {

    class AccessibilityObject;
    class AccessibilityObjectWrapper : public RefCounted<AccessibilityObjectWrapper> {
    public:
        virtual ~AccessibilityObjectWrapper() {}
        virtual void detach() = 0;
        bool attached() const { return m_object; }
        AccessibilityObject* accessibilityObject() const { return m_object; }

    protected:
        AccessibilityObjectWrapper(AccessibilityObject* obj)
            : m_object(obj)
        {
            // FIXME: Remove this once our immediate subclass no longer uses COM.
            *addressOfCount() = 0;
        }
        AccessibilityObjectWrapper() : m_object(0) { }

        AccessibilityObject* m_object;
    };

} // namespace WebCore

#endif
