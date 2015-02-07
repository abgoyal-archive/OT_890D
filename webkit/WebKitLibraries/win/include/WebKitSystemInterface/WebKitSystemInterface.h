

#ifndef WebKitSystemInterface_h
#define WebKitSystemInterface_h

struct CGAffineTransform;
struct CGPoint;
struct CGSize;

typedef const struct __CFData* CFDataRef;
typedef const struct __CFString* CFStringRef;
typedef struct CGColor* CGColorRef;
typedef struct CGContext* CGContextRef;
typedef unsigned short CGFontIndex;
typedef struct CGFont* CGFontRef;
typedef CGFontIndex CGGlyph;
typedef wchar_t UChar;
typedef struct _CFURLResponse* CFURLResponseRef;
typedef struct OpaqueCFHTTPCookieStorage*  CFHTTPCookieStorageRef;
typedef struct _CFURLRequest* CFMutableURLRequestRef;
typedef const struct _CFURLRequest* CFURLRequestRef;

void wkSetFontSmoothingLevel(int type);
int wkGetFontSmoothingLevel();
void wkSetFontSmoothingContrast(CGFloat);
CGFloat wkGetFontSmoothingContrast();
uint32_t wkSetFontSmoothingStyle(CGContextRef cg, bool fontAllowsSmoothing);
void wkRestoreFontSmoothingStyle(CGContextRef cg, uint32_t oldStyle);
void wkSetCGContextFontRenderingStyle(CGContextRef, bool isSystemFont, bool isPrinterFont, bool usePlatformNativeGlyphs);
void wkGetGlyphAdvances(CGFontRef, const CGAffineTransform&, bool isSystemFont, bool isPrinterFont, CGGlyph, CGSize& advance);
void wkGetGlyphs(CGFontRef, const UChar[], CGGlyph[], size_t count);
void wkSetFontPlatformInfo(CGFontRef, LOGFONT*, void(*)(void*));
void wkSetUpFontCache(size_t s);
void wkAddFontsInDirectory(CFStringRef);
void wkAddFontsAtPath(CFStringRef);
void wkAddFontsFromPlistRepresentation(CFDataRef);
CFDataRef wkCreateFontsPlistRepresentation();

void wkSetPatternBaseCTM(CGContextRef, CGAffineTransform);
void wkSetPatternPhaseInUserSpace(CGContextRef, CGPoint phasePoint);

void wkDrawFocusRing(CGContextRef, CGColorRef, float radius);

CFDictionaryRef wkGetSSLCertificateInfo(CFURLResponseRef);
void* wkGetSSLPeerCertificateData(CFDictionaryRef);
CFHTTPCookieStorageRef wkGetDefaultHTTPCookieStorage();
void wkSetCFURLRequestShouldContentSniff(CFMutableURLRequestRef, bool);
CFStringRef wkCopyFoundationCacheDirectory();
void wkSetClientCertificateInSSLProperties(CFMutableDictionaryRef, CFDataRef);

CFArrayRef wkCFURLRequestCopyHTTPRequestBodyParts(CFURLRequestRef);
void wkCFURLRequestSetHTTPRequestBodyParts(CFMutableURLRequestRef, CFArrayRef bodyParts);

unsigned wkInitializeMaximumHTTPConnectionCountPerHost(unsigned preferredConnectionCount);

CFStringRef wkCFNetworkErrorGetLocalizedDescription(CFIndex errorCode);

#endif // WebKitSystemInterface_h
