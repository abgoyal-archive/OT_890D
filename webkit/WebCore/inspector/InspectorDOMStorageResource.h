

#ifndef InspectorDOMStorageResource_h
#define InspectorDOMStorageResource_h

#if ENABLE(DOM_STORAGE)

#include "ScriptObject.h"
#include "ScriptState.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

    class Storage;
    class Frame;
    class InspectorFrontend;

    class InspectorDOMStorageResource : public RefCounted<InspectorDOMStorageResource> {
    public:
        static PassRefPtr<InspectorDOMStorageResource> create(Storage* domStorage, bool isLocalStorage, Frame* frame)
        {
            return adoptRef(new InspectorDOMStorageResource(domStorage, isLocalStorage, frame));
        }

        void bind(InspectorFrontend* frontend);
        void unbind();

        bool isSameHostAndType(Frame*, bool isLocalStorage) const;

    private:

        InspectorDOMStorageResource(Storage*, bool isLocalStorage, Frame*);

        RefPtr<Storage> m_domStorage;
        bool m_isLocalStorage;
        RefPtr<Frame> m_frame;
        bool m_scriptObjectCreated;

    };

} // namespace WebCore

#endif

#endif // InspectorDOMStorageResource_h
