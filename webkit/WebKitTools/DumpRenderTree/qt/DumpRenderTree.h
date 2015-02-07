

#ifndef DUMPRENDERTREE_H
#define DUMPRENDERTREE_H

#include <QList>
#include <QObject>
#include <QTextStream>
#include <QSocketNotifier>

QT_BEGIN_NAMESPACE
class QUrl;
class QFile;
QT_END_NAMESPACE
class QWebPage;
class QWebFrame;

class LayoutTestController;
class EventSender;
class TextInputController;
class GCController;

namespace WebCore {

class DumpRenderTree : public QObject {
Q_OBJECT

public:
    DumpRenderTree();
    virtual ~DumpRenderTree();

    // Initialize in multi-file mode, used by run-webkit-tests.
    void open();

    // Initialize in single-file mode.
    void open(const QUrl& url);

    void setDumpPixels(bool);

    void closeRemainingWindows();
    void resetToConsistentStateBeforeTesting();

    LayoutTestController *layoutTestController() const { return m_controller; }
    EventSender *eventSender() const { return m_eventSender; }
    TextInputController *textInputController() const { return m_textInputController; }

    QWebPage *createWindow();
    int windowCount() const;

    QWebPage *webPage() const { return m_page; }


#if defined(Q_WS_X11)
    static void initializeFonts();
#endif

public Q_SLOTS:
    void initJSObjects();
    void readStdin(int);
    void dump();
    void titleChanged(const QString &s);
    void connectFrame(QWebFrame *frame);
    void dumpDatabaseQuota(QWebFrame* frame, const QString& dbName);

Q_SIGNALS:
    void quit();

private:
    QString dumpFramesAsText(QWebFrame* frame);
    QString dumpBackForwardList();
    LayoutTestController *m_controller;

    bool m_dumpPixels;
    QString m_expectedHash;

    QWebPage *m_page;

    EventSender *m_eventSender;
    TextInputController *m_textInputController;
    GCController* m_gcController;

    QFile *m_stdin;
    QSocketNotifier* m_notifier;

    QList<QWidget *> windows;
};

}

#endif
