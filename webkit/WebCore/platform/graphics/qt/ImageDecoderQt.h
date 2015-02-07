

#ifndef ImageDecoderQt_h
#define ImageDecoderQt_h

#include "ImageDecoder.h"
#include <QtGui/QImage>
#include <QtGui/QPixmap>
#include <QtCore/QList>
#include <QtCore/QHash>

namespace WebCore {


class ImageDecoderQt : public ImageDecoder
{
public:
    static ImageDecoderQt* create(const SharedBuffer& data);
    ~ImageDecoderQt();

    typedef Vector<char> IncomingData;

    virtual void setData(const IncomingData& data, bool allDataReceived);
    virtual bool isSizeAvailable();
    virtual size_t frameCount() const;
    virtual int repetitionCount() const;
    virtual RGBA32Buffer* frameBufferAtIndex(size_t index);

    QPixmap* imageAtIndex(size_t index) const;
    virtual bool supportsAlpha() const;
    int duration(size_t index) const;
    virtual String filenameExtension() const;

    void clearFrame(size_t index);

private:
    ImageDecoderQt(const QString &imageFormat);
    ImageDecoderQt(const ImageDecoderQt&);
    ImageDecoderQt &operator=(const ImageDecoderQt&);

    class ReadContext;
    void reset();
    bool hasFirstImageHeader() const;

    enum ImageState {
        // Started image reading
        ImagePartial,
            // Header (size / alpha) are known
            ImageHeaderValid,
            // Image is complete
            ImageComplete };

    struct ImageData {
        ImageData(const QImage& image, ImageState imageState = ImagePartial, int duration=0);
        QImage m_image;
        ImageState m_imageState;
        int m_duration;
    };

    bool m_hasAlphaChannel;
    typedef QList<ImageData> ImageList;
    mutable ImageList m_imageList;
    mutable QHash<int, QPixmap> m_pixmapCache;
    int m_loopCount;
    QString m_imageFormat;
};



}

#endif

