

#ifndef MediaPlayerPrivateGStreamer_h
#define MediaPlayerPrivateGStreamer_h

#if ENABLE(VIDEO)

#include "MediaPlayerPrivate.h"
#include "Timer.h"

#include <cairo.h>
#include <glib.h>

typedef struct _GstElement GstElement;
typedef struct _GstMessage GstMessage;
typedef struct _GstBus GstBus;

namespace WebCore {

    class GraphicsContext;
    class IntSize;
    class IntRect;
    class String;

    gboolean mediaPlayerPrivateErrorCallback(GstBus* bus, GstMessage* message, gpointer data);
    gboolean mediaPlayerPrivateEOSCallback(GstBus* bus, GstMessage* message, gpointer data);
    gboolean mediaPlayerPrivateStateCallback(GstBus* bus, GstMessage* message, gpointer data);

    class MediaPlayerPrivate : public MediaPlayerPrivateInterface {
        friend gboolean mediaPlayerPrivateErrorCallback(GstBus* bus, GstMessage* message, gpointer data);
        friend gboolean mediaPlayerPrivateEOSCallback(GstBus* bus, GstMessage* message, gpointer data);
        friend gboolean mediaPlayerPrivateStateCallback(GstBus* bus, GstMessage* message, gpointer data);

        public:
            static void registerMediaEngine(MediaEngineRegistrar);
            ~MediaPlayerPrivate();

            IntSize naturalSize() const;
            bool hasVideo() const;

            void load(const String &url);
            void cancelLoad();

            void play();
            void pause();

            bool paused() const;
            bool seeking() const;

            float duration() const;
            float currentTime() const;
            void seek(float);
            void setEndTime(float);

            void setRate(float);
            void setVolume(float);
            void setMuted(bool);

            int dataRate() const;

            MediaPlayer::NetworkState networkState() const;
            MediaPlayer::ReadyState readyState() const;

            float maxTimeBuffered() const;
            float maxTimeSeekable() const;
            unsigned bytesLoaded() const;
            bool totalBytesKnown() const;
            unsigned totalBytes() const;

            void setVisible(bool);
            void setSize(const IntSize&);

            void loadStateChanged();
            void rateChanged();
            void sizeChanged();
            void timeChanged();
            void volumeChanged();
            void didEnd();
            void loadingFailed();

            void repaint();
            void paint(GraphicsContext*, const IntRect&);

        private:
            MediaPlayerPrivate(MediaPlayer*);
            static MediaPlayerPrivateInterface* create(MediaPlayer* player);

            static void getSupportedTypes(HashSet<String>&);
            static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs);
            static bool isAvailable() { return true; }

            void updateStates();
            void cancelSeek();
            void endPointTimerFired(Timer<MediaPlayerPrivate>*);
            float maxTimeLoaded() const;
            void startEndPointTimerIfNeeded();

            void createGSTPlayBin(String url);

        private:
            MediaPlayer* m_player;
            GstElement* m_playBin;
            GstElement* m_videoSink;
            GstElement* m_source;
            float m_rate;
            float m_endTime;
            bool m_isEndReached;
            double m_volume;
            MediaPlayer::NetworkState m_networkState;
            MediaPlayer::ReadyState m_readyState;
            bool m_startedPlaying;
            mutable bool m_isStreaming;
            IntSize m_size;
            bool m_visible;
            cairo_surface_t* m_surface;
    };
}

#endif
#endif
