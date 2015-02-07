

#ifndef QWEBNETWORKINTERFACE_P_H
#define QWEBNETWORKINTERFACE_P_H

#include "qwebnetworkinterface.h"
#if QT_VERSION < 0x040400
#include <qthread.h>
#include <qmutex.h>

namespace WebCore {
    struct HostInfo;
    class ResourceRequest;
};
uint qHash(const WebCore::HostInfo &info);
#include <qhash.h>

namespace WebCore {
    class ResourceHandle;
}

struct QWebNetworkRequestPrivate {
    QUrl url;
    QHttpRequestHeader httpHeader;
    QByteArray postData;

    void init(const WebCore::ResourceRequest &resourceRequest);
    void init(const QString &method, const QUrl &url, const WebCore::ResourceRequest *resourceRequest = 0);
    void setURL(const QUrl &u);
};

class QWebNetworkJobPrivate {
public:
    QWebNetworkJobPrivate()
        : ref(1)
        , resourceHandle(0)
        , redirected(false)
        , interface(0)
        , jobStatus(QWebNetworkJob::JobCreated)
        {}
    int ref;

    QWebNetworkRequestPrivate request;
    QHttpResponseHeader response;

    WebCore::ResourceHandle *resourceHandle;
    bool redirected;

    QWebNetworkInterface *interface;
    QWebNetworkJob::JobStatus jobStatus;
    QString errorString;
};

class QWebNetworkManager : public QObject {
    Q_OBJECT
public:
    enum JobMode {
        AsynchronousJob,
        SynchronousJob
    };

    static QWebNetworkManager *self();

    bool add(WebCore::ResourceHandle *resourceHandle, QWebNetworkInterface *interface, JobMode = AsynchronousJob);
    void cancel(WebCore::ResourceHandle *resourceHandle);

    void addHttpJob(QWebNetworkJob *job);
    void cancelHttpJob(QWebNetworkJob *job);

protected:
    void queueStart(QWebNetworkJob*);
    void queueData(QWebNetworkJob*, const QByteArray&);
    void queueFinished(QWebNetworkJob*, int errorCode);

private:
    void started(QWebNetworkJob *);
    void data(QWebNetworkJob *, const QByteArray &data);
    void finished(QWebNetworkJob *, int errorCode);
    void doScheduleWork();

signals:
    void fileRequest(QWebNetworkJob*);
    void scheduleWork();

private slots:
    void httpConnectionClosed(const WebCore::HostInfo &);
    void doWork();

private:
    friend class QWebNetworkInterface;
    QWebNetworkManager();
    QHash<WebCore::HostInfo, WebCore::WebCoreHttp *> m_hostMapping;

    struct JobWork {
        enum WorkType {
            JobStarted,
            JobData,
            JobFinished
        };

        explicit JobWork(QWebNetworkJob* _job)
            : workType(JobStarted)
            , errorCode(-1)
            , job(_job)
        {}

        explicit JobWork(QWebNetworkJob* _job, int _errorCode)
            : workType(JobFinished)
            , errorCode(_errorCode)
            , job(_job)
        {}

        explicit JobWork(QWebNetworkJob* _job, const QByteArray& _data)
            : workType(JobData)
            , errorCode(-1)
            , job(_job)
            , data(_data)
        {}

        const WorkType workType;
        int errorCode;
        QByteArray data;
        QWebNetworkJob* job;
    };

    QMutex m_queueMutex;
    bool m_scheduledWork;
    QList<JobWork*> m_pendingWork;
    QHash<QWebNetworkJob*, int> m_synchronousJobs;
};


namespace WebCore {

    class NetworkLoader;

    struct HostInfo {
        HostInfo() {}
        HostInfo(const QUrl& url);
        QString protocol;
        QString host;
        int port;
    };

    class WebCoreHttp : public QObject {
        Q_OBJECT
    public:
        WebCoreHttp(QObject* parent, const HostInfo&);
        ~WebCoreHttp();

        void request(QWebNetworkJob* resource);
        void cancel(QWebNetworkJob*);

    signals:
        void connectionClosed(const WebCore::HostInfo&);

    private slots:
        void onResponseHeaderReceived(const QHttpResponseHeader& resp);
        void onReadyRead();
        void onRequestFinished(int, bool);
        void onDone(bool);
        void onStateChanged(int);
        void onSslErrors(const QList<QSslError>&);
        void onAuthenticationRequired(const QString& hostname, quint16 port, QAuthenticator*);
        void onProxyAuthenticationRequired(const QNetworkProxy& proxy, QAuthenticator*);

        void scheduleNextRequest();

        int getConnection();

    public:
        HostInfo info;
    private:
        QList<QWebNetworkJob*> m_pendingRequests;
        struct HttpConnection {
            HttpConnection() : http(0), current(0), id(-1) {}
            QHttp* http;
            QWebNetworkJob* current;
            int id; // the QHttp id
        };
        HttpConnection connection[2];
        bool m_inCancel;
    };

}

class QWebNetworkInterfacePrivate {
public:
    void sendFileData(QWebNetworkJob* job, int statusCode, const QByteArray& data);
    void parseDataUrl(QWebNetworkJob* job);

    QWebNetworkInterface* q;
};

#endif

#endif
