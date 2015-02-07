

#ifndef InspectorClientQt_h
#define InspectorClientQt_h

#include "InspectorClient.h"
#include "OwnPtr.h"
#include <QtCore/QString>

class QWebPage;

namespace WebCore {
    class Node;
    class Page;
    class String;
    class InspectorClientWebPage;

    class InspectorClientQt : public InspectorClient {
    public:
        InspectorClientQt(QWebPage*);

        virtual void inspectorDestroyed();

        virtual Page* createPage();

        virtual String localizedStringsURL();

        virtual String hiddenPanels();

        virtual void showWindow();
        virtual void closeWindow();
        virtual bool windowVisible();

        virtual void attachWindow();
        virtual void detachWindow();

        virtual void setAttachedWindowHeight(unsigned height);

        virtual void highlight(Node*);
        virtual void hideHighlight();
        virtual void inspectedURLChanged(const String& newURL);

        virtual void populateSetting(const String& key, InspectorController::Setting&);
        virtual void storeSetting(const String& key, const InspectorController::Setting&);
        virtual void removeSetting(const String& key);

        virtual void inspectorWindowObjectCleared();

    private:
        void updateWindowTitle();
        QWebPage* m_inspectedWebPage;
        OwnPtr<InspectorClientWebPage> m_webPage;
        QString m_inspectedURL;
    };
}

#endif
