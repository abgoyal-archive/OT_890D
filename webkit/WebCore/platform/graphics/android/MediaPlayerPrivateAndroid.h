

#ifndef MediaPlayerPrivateAndroid_h
#define MediaPlayerPrivateAndroid_h

#if ENABLE(VIDEO)

class SkBitmap;

#include "MediaPlayerPrivate.h"

namespace WebCore {

class MediaPlayerPrivate : public MediaPlayerPrivateInterface {
public:
    ~MediaPlayerPrivate();

    static void registerMediaEngine(MediaEngineRegistrar);

    virtual void load(const String& url);
    virtual void cancelLoad();

    virtual void play();
    virtual void pause();

    virtual IntSize naturalSize() const;

    virtual bool hasVideo() const;

    virtual void setVisible(bool);

    virtual float duration() const;

    virtual float currentTime() const;
    virtual void seek(float time);
    virtual bool seeking() const;

    virtual void setEndTime(float time);

    virtual void setRate(float);
    virtual bool paused() const;

    virtual void setVolume(float);

    virtual MediaPlayer::NetworkState networkState() const;
    virtual MediaPlayer::ReadyState readyState() const;

    virtual float maxTimeSeekable() const;
    virtual float maxTimeBuffered() const;

    virtual int dataRate() const;

    virtual bool totalBytesKnown() const { return totalBytes() > 0; }
    virtual unsigned totalBytes() const;
    virtual unsigned bytesLoaded() const;

    virtual void setSize(const IntSize&);

    virtual bool canLoadPoster() const { return true; }
    virtual void setPoster(const String&);
    virtual void prepareToPlay();

    virtual void paint(GraphicsContext*, const IntRect&);

    void onPrepared(int duration, int width, int height);
    void onEnded();
    void onPosterFetched(SkBitmap*);
private:
    // Android-specific methods and fields.
    static MediaPlayerPrivateInterface* create(MediaPlayer* player);
    static void getSupportedTypes(HashSet<String>&);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs);

    MediaPlayerPrivate(MediaPlayer *);
    void createJavaPlayerIfNeeded();

    MediaPlayer* m_player;
    String m_url;
    struct JavaGlue;
    JavaGlue* m_glue;

    float m_duration;
    float m_currentTime;

    bool m_paused;
    MediaPlayer::ReadyState m_readyState;
    MediaPlayer::NetworkState m_networkState;

    SkBitmap* m_poster;  // not owned
    String m_posterUrl;

    IntSize m_naturalSize;
    bool m_naturalSizeUnknown;

    bool m_isVisible;
};

}  // namespace WebCore

#endif

#endif // MediaPlayerPrivateAndroid_h
