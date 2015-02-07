

#ifndef WorkQueueItem_h
#define WorkQueueItem_h

#include <qstring.h>
#include <qpointer.h>
#include <qwebpage.h>

class WorkQueueItem {
public:
    WorkQueueItem(QWebPage *page) : m_webPage(page) {}
    virtual ~WorkQueueItem() { }
    virtual bool invoke() const = 0;

protected:
    QPointer<QWebPage> m_webPage;
};

class LoadItem : public WorkQueueItem {
public:
    LoadItem(const QString &url, const QString &target, QWebPage *page)
        : WorkQueueItem(page)
        , m_url(url)
        , m_target(target)
    {
    }

    QString url() const { return m_url; }
    QString target() const { return m_target; }

    virtual bool invoke() const;

private:
    QString m_url;
    QString m_target;
};

class ReloadItem : public WorkQueueItem {
public:
    ReloadItem(QWebPage *page)
        : WorkQueueItem(page)
    {
    }
    virtual bool invoke() const;
};

class ScriptItem : public WorkQueueItem {
public:
    ScriptItem(const QString &script, QWebPage *page)
        : WorkQueueItem(page)
        , m_script(script)
    {
    }

    QString script() const { return m_script; }

    virtual bool invoke() const;

private:
    QString m_script;
};

class BackForwardItem : public WorkQueueItem {
public:
    virtual bool invoke() const;

protected:
    BackForwardItem(int howFar, QWebPage *page)
        : WorkQueueItem(page)
        , m_howFar(howFar)
    {
    }

    int m_howFar;
};

class BackItem : public BackForwardItem {
public:
    BackItem(unsigned howFar, QWebPage *page)
        : BackForwardItem(-howFar, page)
    {
    }
};

class ForwardItem : public BackForwardItem {
public:
    ForwardItem(unsigned howFar, QWebPage *page)
        : BackForwardItem(howFar, page)
    {
    }
};

#endif // !defined(WorkQueueItem_h)
