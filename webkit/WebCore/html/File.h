

#ifndef File_h
#define File_h

#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

    class File : public RefCounted<File> {
    public:
        static PassRefPtr<File> create(const String& path)
        {
            return adoptRef(new File(path));
        }

        const String& fileName() const { return m_fileName; }
        unsigned long long fileSize();

        const String& path() const { return m_path; }

    private:
        File(const String& path);

        String m_path;
        String m_fileName;
    };

} // namespace WebCore

#endif // FileList_h
