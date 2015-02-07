
 
#ifndef FileSystem_h
#define FileSystem_h

#if PLATFORM(GTK)
#include <gmodule.h>
#endif
#if PLATFORM(QT)
#include <QFile>
#include <QLibrary>
#if defined(Q_OS_WIN32)
#include <windows.h>
#endif
#if defined(Q_WS_MAC)
#include <CoreFoundation/CFBundle.h>
#endif
#endif

#include <time.h>

#include <wtf/Platform.h>
#include <wtf/Vector.h>

#include "PlatformString.h"

typedef const struct __CFData* CFDataRef;

#if PLATFORM(WIN_OS)
// These are to avoid including <winbase.h> in a header for Chromium
typedef void *HANDLE;
// Assuming STRICT
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
#endif

namespace WebCore {

class CString;

#if PLATFORM(QT)

typedef QFile* PlatformFileHandle;
const PlatformFileHandle invalidPlatformFileHandle = 0;
#if defined(Q_WS_MAC)
typedef CFBundleRef PlatformModule;
typedef unsigned PlatformModuleVersion;
#elif defined(Q_OS_WIN)
typedef HMODULE PlatformModule;
struct PlatformModuleVersion {
    unsigned leastSig;
    unsigned mostSig;

    PlatformModuleVersion(unsigned)
        : leastSig(0)
        , mostSig(0)
    {
    }

    PlatformModuleVersion(unsigned lsb, unsigned msb)
        : leastSig(lsb)
        , mostSig(msb)
    {
    }

};
#else
typedef QLibrary* PlatformModule;
typedef unsigned PlatformModuleVersion;
#endif

#elif PLATFORM(WIN_OS)
typedef HANDLE PlatformFileHandle;
typedef HMODULE PlatformModule;
// FIXME: -1 is INVALID_HANDLE_VALUE, defined in <winbase.h>. Chromium tries to
// avoid using Windows headers in headers.  We'd rather move this into the .cpp.
const PlatformFileHandle invalidPlatformFileHandle = reinterpret_cast<HANDLE>(-1);

struct PlatformModuleVersion {
    unsigned leastSig;
    unsigned mostSig;

    PlatformModuleVersion(unsigned)
        : leastSig(0)
        , mostSig(0)
    {
    }

    PlatformModuleVersion(unsigned lsb, unsigned msb)
        : leastSig(lsb)
        , mostSig(msb)
    {
    }

};
#else
typedef int PlatformFileHandle;
#if PLATFORM(GTK)
typedef GModule* PlatformModule;
#else
typedef void* PlatformModule;
#endif
const PlatformFileHandle invalidPlatformFileHandle = -1;

typedef unsigned PlatformModuleVersion;
#endif

bool fileExists(const String&);
bool deleteFile(const String&);
bool deleteEmptyDirectory(const String&);
bool getFileSize(const String&, long long& result);
bool getFileModificationTime(const String&, time_t& result);
String pathByAppendingComponent(const String& path, const String& component);
bool makeAllDirectories(const String& path);
String homeDirectoryPath();
String pathGetFileName(const String&);
String directoryName(const String&);

Vector<String> listDirectory(const String& path, const String& filter = String());

CString fileSystemRepresentation(const String&);

inline bool isHandleValid(const PlatformFileHandle& handle) { return handle != invalidPlatformFileHandle; }

// Prefix is what the filename should be prefixed with, not the full path.
CString openTemporaryFile(const char* prefix, PlatformFileHandle&);
void closeFile(PlatformFileHandle&);
int writeToFile(PlatformFileHandle, const char* data, int length);

// Methods for dealing with loadable modules
bool unloadModule(PlatformModule);

#if PLATFORM(WIN)
String localUserSpecificStorageDirectory();
String roamingUserSpecificStorageDirectory();

bool safeCreateFile(const String&, CFDataRef);
#endif

#if PLATFORM(GTK)
String filenameToString(const char*);
char* filenameFromString(const String&);
String filenameForDisplay(const String&);
#endif

} // namespace WebCore

#endif // FileSystem_h
