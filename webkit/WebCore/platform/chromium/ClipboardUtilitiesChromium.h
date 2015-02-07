

namespace WebCore {

class KURL;
class String;

#if PLATFORM(WIN_OS)
void replaceNewlinesWithWindowsStyleNewlines(String&);
#endif
void replaceNBSPWithSpace(String&);

String urlToMarkup(const KURL&, const String&);

} // namespace WebCore
