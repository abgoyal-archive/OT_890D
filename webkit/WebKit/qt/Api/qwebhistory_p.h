

#ifndef QWEBHISTORY_P_H
#define QWEBHISTORY_P_H

#include "BackForwardList.h"
#include "HistoryItem.h"
#include <QtCore/qglobal.h>
#include <QtCore/qshareddata.h>

class Q_AUTOTEST_EXPORT QWebHistoryItemPrivate : public QSharedData {
public:
    static QExplicitlySharedDataPointer<QWebHistoryItemPrivate> get(QWebHistoryItem *q)
    {
        return q->d;
    }
    QWebHistoryItemPrivate(WebCore::HistoryItem *i)
    {
        if (i)
            i->ref();
        item = i;
    }
    ~QWebHistoryItemPrivate()
    {
        if (item)
            item->deref();
    }

 /*   QByteArray saveStateWithoutVersionControl(QWebHistory::HistoryStateVersion version)
    {
        QByteArray buffer;
        switch(version){
            case QWebHistory::HistoryVersion1:
                buffer=item->saveState(version);
                break;
            default:{}
        }
        return buffer;
    }

    bool restoreStateWithoutVersionControl(QWebHistory::HistoryStateVersion version,QDataStream& stream)
    {

    }
*/

    WebCore::HistoryItem *item;
};

class QWebHistoryPrivate : public QSharedData {
public:
    QWebHistoryPrivate(WebCore::BackForwardList *l)
    {
        l->ref();
        lst = l;
    }
    ~QWebHistoryPrivate()
    {
        lst->deref();
    }
    WebCore::BackForwardList *lst;
};


#endif
