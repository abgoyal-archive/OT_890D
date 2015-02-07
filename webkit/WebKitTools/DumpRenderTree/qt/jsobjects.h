
#ifndef JSOBJECTS_H
#define JSOBJECTS_H

#include <qobject.h>
#include <qdebug.h>
#include <qpoint.h>
#include <qstringlist.h>
#include <qsize.h>
#include <qbasictimer.h>

class QWebFrame;
namespace WebCore {
    class DumpRenderTree;
}
class LayoutTestController : public QObject
{
    Q_OBJECT
public:
    LayoutTestController(WebCore::DumpRenderTree *drt);

    bool isLoading() const { return m_isLoading; }
    void setLoading(bool loading) { m_isLoading = loading; }

    bool shouldDumpAsText() const { return m_textDump; }
    bool shouldDumpBackForwardList() const { return m_dumpBackForwardList; }
    bool shouldDumpChildrenAsText() const { return m_dumpChildrenAsText; }
    bool shouldDumpDatabaseCallbacks() const { return m_dumpDatabaseCallbacks; }
    bool shouldWaitUntilDone() const { return m_waitForDone; }
    bool canOpenWindows() const { return m_canOpenWindows; }
    bool shouldDumpTitleChanges() const { return m_dumpTitleChanges; }

    void reset();

protected:
    void timerEvent(QTimerEvent *);

signals:
    void done();

public slots:
    void maybeDump(bool ok);
    void dumpAsText() { m_textDump = true; }
    void dumpChildFramesAsText() { m_dumpChildrenAsText = true; }
    void dumpDatabaseCallbacks() { m_dumpDatabaseCallbacks = true; }
    void setCanOpenWindows() { m_canOpenWindows = true; }
    void waitUntilDone();
    void notifyDone();
    void dumpBackForwardList() { m_dumpBackForwardList = true; }
    void dumpEditingCallbacks();
    void dumpResourceLoadCallbacks();
    void queueBackNavigation(int howFarBackward);
    void queueForwardNavigation(int howFarForward);
    void queueLoad(const QString &url, const QString &target = QString());
    void queueReload();
    void queueScript(const QString &url);
    void provisionalLoad();
    void setCloseRemainingWindowsWhenComplete(bool=false) {}
    int windowCount();
    void display() {}
    void clearBackForwardList();
    void dumpTitleChanges() { m_dumpTitleChanges = true; }
    QString encodeHostName(const QString &host);
    QString decodeHostName(const QString &host);
    void dumpSelectionRect() const {}
    void setJavaScriptProfilingEnabled(bool enable);
    void setFixedContentsSize(int width, int height);
    void setPrivateBrowsingEnabled(bool enable);

    bool pauseAnimationAtTimeOnElementWithId(const QString &animationName, double time, const QString &elementId);
    bool pauseTransitionAtTimeOnElementWithId(const QString &propertyName, double time, const QString &elementId);
    unsigned numberOfActiveAnimations() const;
    void dispatchPendingLoadRequests();
    void disableImageLoading();

    void setDatabaseQuota(int size);
    void clearAllDatabases();

private slots:
    void processWork();

private:
    bool m_isLoading;
    bool m_textDump;
    bool m_dumpBackForwardList;
    bool m_dumpChildrenAsText;
    bool m_canOpenWindows;
    bool m_waitForDone;
    bool m_dumpTitleChanges;
    bool m_dumpDatabaseCallbacks;
    QBasicTimer m_timeoutTimer;
    QWebFrame *m_topLoadingFrame;
    WebCore::DumpRenderTree *m_drt;
};

class QWebPage;
class QWebFrame;

class EventSender : public QObject
{
    Q_OBJECT
public:
    EventSender(QWebPage *parent);

public slots:
    void mouseDown();
    void mouseUp();
    void mouseMoveTo(int x, int y);
    void leapForward(int ms);
    void keyDown(const QString &string, const QStringList &modifiers=QStringList());
    void clearKillRing() {}

private:
    QPoint m_mousePos;
    QWebPage *m_page;
    int m_timeLeap;
    QWebFrame *frameUnderMouse() const;
};

class TextInputController : public QObject
{
    Q_OBJECT
public:
    TextInputController(QWebPage *parent);

public slots:
    void doCommand(const QString &command);
//     void setMarkedText(const QString &str, int from, int length);
//     bool hasMarkedText();
//     void unmarkText();
//     QList<int> markedRange();
//     QList<int> selectedRange();
//     void validAttributesForMarkedText();
//     void inserText(const QString &);
//     void firstRectForCharacterRange();
//     void characterIndexForPoint(int, int);
//     void substringFromRange(int, int);
//     void conversationIdentifier();
};

class GCController : public QObject
{
    Q_OBJECT
public:
    GCController(QWebPage* parent);

public slots:
    void collect() const;
    void collectOnAlternateThread(bool waitUntilDone) const;
    size_t getJSObjectCount() const;
};

#endif
