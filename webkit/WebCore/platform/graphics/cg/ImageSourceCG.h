

#ifndef ImageSourceCG_h
#define ImageSourceCG_h

#include "ImageSource.h"

namespace WebCore {

class String;

String preferredExtensionForImageSourceType(const String& type);

String MIMETypeForImageSourceType(const String& type);

}

#endif // ImageSourceCG_h
