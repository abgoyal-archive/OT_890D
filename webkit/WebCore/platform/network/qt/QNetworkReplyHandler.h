
#ifndef QNETWORKREPLYHANDLER_H
#define QNETWORKREPLYHANDLER_H

#include <QObject>

#if QT_VERSION >= 0x040400

#include <QNetworkRequest>
#include <QNetworkAccessManager>

#include "FormData.h"

QT_BEGIN_NAMESPACE
class QFile;
class QNetworkReply;
QT_END_NAMESPACE

namespace WebCore {

class ResourceHandle;

class QNetworkReplyHandler : public QObject
{
    Q_OBJECT
public:
    enum LoadMode {
        LoadNormal,
        LoadDeferred,
        LoadResuming
    };

    QNetworkReplyHandler(ResourceHandle *handle, LoadMode);
    void setLoadMode(LoadMode);

    QNetworkReply* reply() const { return m_reply; }

    void abort();

    QNetworkReply* release();

signals:
    void processQueuedItems();

private slots:
    void finish();
    void sendResponseIfNeeded();
    void forwardData();
    void sendQueuedItems();

private:
    void start();
    void resetState();

    QNetworkReply* m_reply;
    ResourceHandle* m_resourceHandle;
    bool m_redirected;
    bool m_responseSent;
    LoadMode m_loadMode;
    QNetworkAccessManager::Operation m_method;
    QNetworkRequest m_request;

    // defer state holding
    bool m_shouldStart;
    bool m_shouldFinish;
    bool m_shouldSendResponse;
    bool m_shouldForwardData;
};

// Self destructing QIODevice for FormData
//  For QNetworkAccessManager::put we will have to gurantee that the
//  QIODevice is valid as long finished() of the QNetworkReply has not
//  been emitted. With the presence of QNetworkReplyHandler::release I do
//  not want to gurantee this.
class FormDataIODevice : public QIODevice {
    Q_OBJECT
public:
    FormDataIODevice(FormData*);
    ~FormDataIODevice();

    void setParent(QNetworkReply*);
    bool isSequential() const;

protected:
    qint64 readData(char*, qint64);
    qint64 writeData(const char*, qint64);

private Q_SLOTS:
    void slotFinished();

private:
    void moveToNextElement();

private:
    Vector<FormDataElement> m_formElements;
    QFile* m_currentFile;
    qint64 m_currentDelta;
};

}

#endif

#endif // QNETWORKREPLYHANDLER_H
