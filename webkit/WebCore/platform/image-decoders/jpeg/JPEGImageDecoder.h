

#ifndef JPEGImageDecoder_h
#define JPEGImageDecoder_h

#include "ImageDecoder.h"

namespace WebCore {

    class JPEGImageReader;

    // This class decodes the JPEG image format.
    class JPEGImageDecoder : public ImageDecoder {
    public:
        JPEGImageDecoder();
        ~JPEGImageDecoder();

        virtual String filenameExtension() const { return "jpg"; }

        // Take the data and store it.
        virtual void setData(SharedBuffer* data, bool allDataReceived);

        // Whether or not the size information has been decoded yet.
        virtual bool isSizeAvailable();

        virtual RGBA32Buffer* frameBufferAtIndex(size_t index);
        
        virtual bool supportsAlpha() const { return false; }

        void decode(bool sizeOnly = false);

        JPEGImageReader* reader() { return m_reader; }

        bool outputScanlines();
        void jpegComplete();

    private:
        JPEGImageReader* m_reader;
    };

} // namespace WebCore

#endif
