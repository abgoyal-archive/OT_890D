

#ifndef CookieJar_h
#define CookieJar_h

namespace WebCore {

    class KURL;
    class String;
    class Document;

    String cookies(const Document*, const KURL&);
    void setCookies(Document*, const KURL&, const String&);
    bool cookiesEnabled(const Document*);

}

#endif
