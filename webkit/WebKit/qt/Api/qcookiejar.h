

#ifndef QCOOKIEJAR_H
#define QCOOKIEJAR_H

#include <QtCore/qobject.h>
#include <QtCore/qurl.h>
#include "qwebkitglobal.h"

class QCookieJarPrivate;

class QWEBKIT_EXPORT QCookieJar : public QObject {
    Q_OBJECT

    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
public:
    QCookieJar();
    ~QCookieJar();

    virtual void setCookies(const QUrl& url, const QUrl& policyUrl, const QString& value);
    virtual QString cookies(const QUrl& url);

    bool isEnabled() const;

    static void setCookieJar(QCookieJar* jar);
    static QCookieJar* cookieJar();

public slots:
    virtual void setEnabled(bool enabled);

private:
    friend class QCookieJarPrivate;
    QCookieJarPrivate* d;
};


#endif
