
#ifndef SharedBuffer_h
#define SharedBuffer_h

#include "PlatformString.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

#if PLATFORM(CF)
#include <wtf/RetainPtr.h>
#endif

#if PLATFORM(MAC)
#ifdef __OBJC__
@class NSData;
#else
class NSData;
#endif

#endif

namespace WebCore {
    
class PurgeableBuffer;

class SharedBuffer : public RefCounted<SharedBuffer> {
public:
    static PassRefPtr<SharedBuffer> create() { return adoptRef(new SharedBuffer); }
    static PassRefPtr<SharedBuffer> create(const char* c, int i) { return adoptRef(new SharedBuffer(c, i)); }
    static PassRefPtr<SharedBuffer> create(const unsigned char* c, int i) { return adoptRef(new SharedBuffer(c, i)); }

    static PassRefPtr<SharedBuffer> createWithContentsOfFile(const String& filePath);

    static PassRefPtr<SharedBuffer> adoptVector(Vector<char>& vector);
    
    // The buffer must be in non-purgeable state before adopted to a SharedBuffer. 
    // It will stay that way until released.
    static PassRefPtr<SharedBuffer> adoptPurgeableBuffer(PurgeableBuffer* buffer);

#if PLATFORM(ANDROID)
    virtual
#endif
    ~SharedBuffer();
    
#if PLATFORM(MAC)
    NSData *createNSData();
    static PassRefPtr<SharedBuffer> wrapNSData(NSData *data);
#endif
#if PLATFORM(CF)
    CFDataRef createCFData();
    static PassRefPtr<SharedBuffer> wrapCFData(CFDataRef);
#endif

#if PLATFORM(ANDROID)
    virtual
#endif
    const char* data() const;
#if PLATFORM(ANDROID)
    virtual
#endif
    unsigned size() const;
    const Vector<char> &buffer() { return m_buffer; }

    bool isEmpty() const { return size() == 0; }

    void append(const char*, int);
    void clear();
    const char* platformData() const;
    unsigned platformDataSize() const;

    PassRefPtr<SharedBuffer> copy() const;
    
    bool hasPurgeableBuffer() const { return m_purgeableBuffer.get(); }

    // Ensure this buffer has no other clients before calling this.
    PurgeableBuffer* releasePurgeableBuffer();
    
private:
    SharedBuffer();
    SharedBuffer(const char*, int);
    SharedBuffer(const unsigned char*, int);
    
    void clearPlatformData();
    void maybeTransferPlatformData();
    bool hasPlatformData() const;
    
    Vector<char> m_buffer;
    OwnPtr<PurgeableBuffer> m_purgeableBuffer;
#if PLATFORM(CF)
    SharedBuffer(CFDataRef);
    RetainPtr<CFDataRef> m_cfData;
#endif
};
    
}

#endif
