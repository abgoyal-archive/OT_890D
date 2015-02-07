

#ifndef QTMovieWin_h
#define QTMovieWin_h

#include <Unicode.h>

#ifdef QTMOVIEWIN_EXPORTS
#define QTMOVIEWIN_API __declspec(dllexport)
#else
#define QTMOVIEWIN_API __declspec(dllimport)
#endif

class QTMovieWin;
class QTMovieWinPrivate;

class QTMovieWinClient {
public:
    virtual void movieEnded(QTMovieWin*) = 0;
    virtual void movieLoadStateChanged(QTMovieWin*) = 0;
    virtual void movieTimeChanged(QTMovieWin*) = 0;
    virtual void movieNewImageAvailable(QTMovieWin*) = 0;
};

enum {
    QTMovieLoadStateError = -1L,
    QTMovieLoadStateLoaded  = 2000L,
    QTMovieLoadStatePlayable = 10000L,
    QTMovieLoadStatePlaythroughOK = 20000L,
    QTMovieLoadStateComplete = 100000L
};

typedef const struct __CFURL * CFURLRef;

class QTMOVIEWIN_API QTMovieWin {
public:
    static bool initializeQuickTime();

    QTMovieWin(QTMovieWinClient*);
    ~QTMovieWin();

    void load(const UChar* url, int len, bool preservesPitch);
    long loadState() const;
    float maxTimeLoaded() const;

    void play();
    void pause();

    float rate() const;
    void setRate(float);

    float duration() const;
    float currentTime() const;
    void setCurrentTime(float) const;

    void setVolume(float);
    void setPreservesPitch(bool);

    unsigned dataSize() const;

    void getNaturalSize(int& width, int& height);
    void setSize(int width, int height);

    void setVisible(bool);
    void paint(HDC, int x, int y);

    void disableUnsupportedTracks(unsigned& enabledTrackCount, unsigned& totalTrackCount);
    void setDisabled(bool);

    bool hasVideo() const;

    static unsigned countSupportedTypes();
    static void getSupportedType(unsigned index, const UChar*& str, unsigned& len);

private:
    void load(CFURLRef, bool preservesPitch);

    QTMovieWinPrivate* m_private;
    bool m_disabled;
    friend class QTMovieWinPrivate;
};

#endif
