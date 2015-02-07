

#ifndef AXObjectCache_h
#define AXObjectCache_h

#include "AccessibilityObject.h"
#include "EventHandler.h"
#include "Timer.h"
#include <limits.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

#ifdef __OBJC__
@class WebCoreTextMarker;
#else
class WebCoreTextMarker;
#endif

namespace WebCore {

    class RenderObject;
    class String;
    class VisiblePosition;
    class AccessibilityObject;
    class Node;
    
    typedef unsigned AXID;

    struct TextMarkerData  {
        AXID axID;
        Node* node;
        int offset;
        EAffinity affinity;
    };

    class AXObjectCache {
    public:
        AXObjectCache();
        ~AXObjectCache();
        
        // to be used with render objects
        AccessibilityObject* getOrCreate(RenderObject*);
        
        // used for objects without backing elements
        AccessibilityObject* getOrCreate(AccessibilityRole);
        
        // will only return the AccessibilityObject if it already exists
        AccessibilityObject* get(RenderObject*);
        
        void remove(RenderObject*);
        void remove(AXID);

        void detachWrapper(AccessibilityObject*);
        void attachWrapper(AccessibilityObject*);
        void postNotification(RenderObject*, const String&, bool postToElement);
        void postPlatformNotification(AccessibilityObject*, const String&);
        void childrenChanged(RenderObject*);
        void selectedChildrenChanged(RenderObject*);
        void handleActiveDescendantChanged(RenderObject*);
        void handleAriaRoleChanged(RenderObject*);
        void handleFocusedUIElementChanged();
#if PLATFORM(GTK)
        void handleFocusedUIElementChangedWithRenderers(RenderObject*, RenderObject*);
#endif
        static void enableAccessibility() { gAccessibilityEnabled = true; }
        static void enableEnhancedUserInterfaceAccessibility() { gAccessibilityEnhancedUserInterfaceEnabled = true; }
        
        static bool accessibilityEnabled() { return gAccessibilityEnabled; }
        static bool accessibilityEnhancedUserInterfaceEnabled() { return gAccessibilityEnhancedUserInterfaceEnabled; }

        void removeAXID(AccessibilityObject*);
        bool isIDinUse(AXID id) const { return m_idsInUse.contains(id); }

        // Text marker utilities.
        static void textMarkerDataForVisiblePosition(TextMarkerData&, const VisiblePosition&);
        static VisiblePosition visiblePositionForTextMarkerData(TextMarkerData&);
                
    private:
        HashMap<AXID, RefPtr<AccessibilityObject> > m_objects;
        HashMap<RenderObject*, AXID> m_renderObjectMapping;
        static bool gAccessibilityEnabled;
        static bool gAccessibilityEnhancedUserInterfaceEnabled;
        
        HashSet<AXID> m_idsInUse;
        
        Timer<AXObjectCache> m_notificationPostTimer;
        Vector<pair<RefPtr<AccessibilityObject>, const String> > m_notificationsToPost;
        void notificationPostTimerFired(Timer<AXObjectCache>*);
        
        AXID getAXID(AccessibilityObject*);
        bool nodeIsAriaType(Node* node, String role);
    };

#if !HAVE(ACCESSIBILITY)
    inline void AXObjectCache::handleActiveDescendantChanged(RenderObject*) { }
    inline void AXObjectCache::handleAriaRoleChanged(RenderObject*) { }
    inline void AXObjectCache::handleFocusedUIElementChanged() { }
    inline void AXObjectCache::detachWrapper(AccessibilityObject*) { }
    inline void AXObjectCache::attachWrapper(AccessibilityObject*) { }
    inline void AXObjectCache::selectedChildrenChanged(RenderObject*) { }
    inline void AXObjectCache::postNotification(RenderObject*, const String&, bool postToElement) { }
    inline void AXObjectCache::postPlatformNotification(AccessibilityObject*, const String&) { }
#if PLATFORM(GTK)
    inline void AXObjectCache::handleFocusedUIElementChangedWithRenderers(RenderObject*, RenderObject*) { }
#endif
#endif

}

#endif
