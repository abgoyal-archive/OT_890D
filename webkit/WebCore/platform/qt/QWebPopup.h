
#ifndef QWebPopup_h
#define QWebPopup_h

#include <QComboBox>

#include "PopupMenuClient.h"

namespace WebCore {

class QWebPopup : public QComboBox {
    Q_OBJECT
public:
    QWebPopup(PopupMenuClient* client);

    void exec();

    virtual void showPopup();
    virtual void hidePopup();

private slots:
    void activeChanged(int);
private:
    PopupMenuClient* m_client;
    bool m_popupVisible;
};

}

#endif
