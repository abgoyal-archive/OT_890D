

#ifndef MediaPlayerPrivateQTKit_h
#define MediaPlayerPrivateQTKit_h

#if ENABLE(VIDEO)

#include "MediaPlayerPrivate.h"
#include "Timer.h"
#include "FloatSize.h"
#include <wtf/RetainPtr.h>

#ifdef __OBJC__
#import <QTKit/QTTime.h>
@class QTMovie;
@class QTMovieView;
@class QTMovieLayer;
@class QTVideoRendererWebKitOnly;
@class WebCoreMovieObserver;
#else
class QTMovie;
class QTMovieView;
class QTTime;
class QTMovieLayer;
class QTVideoRendererWebKitOnly;
class WebCoreMovieObserver;
#endif

#ifndef DRAW_FRAME_RATE
#define DRAW_FRAME_RATE 0
#endif

namespace WebCore {

class MediaPlayerPrivate : public MediaPlayerPrivateInterface {
public:
    static void registerMediaEngine(MediaEngineRegistrar);

    ~MediaPlayerPrivate();

    void repaint();
    void loadStateChanged();
    void rateChanged();
    void sizeChanged();
    void timeChanged();
    void didEnd();

private:
    MediaPlayerPrivate(MediaPlayer*);

    // engine support
    static MediaPlayerPrivateInterface* create(MediaPlayer* player);
    static void getSupportedTypes(HashSet<String>& types);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs);
    static bool isAvailable();

    IntSize naturalSize() const;
    bool hasVideo() const;
    
    void load(const String& url);
    void cancelLoad();
    
    void play();
    void pause();    
    
    bool paused() const;
    bool seeking() const;
    
    float duration() const;
    float currentTime() const;
    void seek(float time);
    
    void setRate(float);
    void setVolume(float);
    void setPreservesPitch(bool);

    void setEndTime(float time);

    int dataRate() const;
    
    MediaPlayer::NetworkState networkState() const { return m_networkState; }
    MediaPlayer::ReadyState readyState() const { return m_readyState; }
    
    float maxTimeBuffered() const;
    float maxTimeSeekable() const;
    unsigned bytesLoaded() const;
    bool totalBytesKnown() const;
    unsigned totalBytes() const;
    
    void setVisible(bool);
    void setSize(const IntSize&);
    
    void paint(GraphicsContext*, const IntRect&);
    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);

#if USE(ACCELERATED_COMPOSITING)
    bool supportsAcceleratedRendering() const;
    void acceleratedRenderingStateChanged();
#endif

    bool hasSingleSecurityOrigin() const;
    MediaPlayer::MovieLoadType movieLoadType() const;

    void createQTMovie(const String& url);
    void createQTMovie(NSURL *, NSDictionary *movieAttributes);

    enum MediaRenderingMode { MediaRenderingNone, MediaRenderingMovieView, MediaRenderingSoftwareRenderer, MediaRenderingMovieLayer };
    MediaRenderingMode currentRenderingMode() const;
    MediaRenderingMode preferredRenderingMode() const;
    
    void setUpVideoRendering();
    void tearDownVideoRendering();
    bool hasSetUpVideoRendering() const;
    
    void createQTMovieView();
    void detachQTMovieView();
    
    enum QTVideoRendererMode { QTVideoRendererModeDefault, QTVideoRendererModeListensForNewImages };
    void createQTVideoRenderer(QTVideoRendererMode rendererMode);
    void destroyQTVideoRenderer();
    
    void createQTMovieLayer();
    void destroyQTMovieLayer();

    QTTime createQTTime(float time) const;
    
    void updateStates();
    void doSeek();
    void cancelSeek();
    void seekTimerFired(Timer<MediaPlayerPrivate>*);
    float maxTimeLoaded() const;
    void disableUnsupportedTracks();
    
    void sawUnsupportedTracks();
    void cacheMovieScale();
    bool metaDataAvailable() const { return m_qtMovie && m_readyState >= MediaPlayer::HaveMetadata; }

    bool isReadyForRendering() const;
    
    MediaPlayer* m_player;
    RetainPtr<QTMovie> m_qtMovie;
    RetainPtr<QTMovieView> m_qtMovieView;
    RetainPtr<QTVideoRendererWebKitOnly> m_qtVideoRenderer;
    RetainPtr<WebCoreMovieObserver> m_objcObserver;
    float m_seekTo;
    Timer<MediaPlayerPrivate> m_seekTimer;
    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;
    bool m_startedPlaying;
    bool m_isStreaming;
    bool m_visible;
    IntRect m_rect;
    FloatSize m_scaleFactor;
    unsigned m_enabledTrackCount;
    unsigned m_totalTrackCount;
    bool m_hasUnsupportedTracks;
    float m_reportedDuration;
    float m_cachedDuration;
    float m_timeToRestore;
    RetainPtr<QTMovieLayer> m_qtVideoLayer;
#if DRAW_FRAME_RATE
    int  m_frameCountWhilePlaying;
    double m_timeStartedPlaying;
    double m_timeStoppedPlaying;
#endif
};

}

#endif
#endif
