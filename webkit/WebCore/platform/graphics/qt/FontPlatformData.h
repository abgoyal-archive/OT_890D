
#ifndef FontPlatformData_h
#define FontPlatformData_h

#include "FontDescription.h"
#include <QFont>

namespace WebCore {

class String;

class FontPlatformData
{
public:
#if ENABLE(SVG_FONTS)
    FontPlatformData(float size, bool bold, bool oblique);
#endif
    FontPlatformData();
    FontPlatformData(const FontDescription&, int wordSpacing = 0, int letterSpacing = 0);
    FontPlatformData(const QFont&, bool bold);

    static inline QFont::Weight toQFontWeight(FontWeight fontWeight)
    {
        switch (fontWeight) {
        case FontWeight100:
        case FontWeight200:
            return QFont::Light;  // QFont::Light == Weight of 25
        case FontWeight600:
            return QFont::DemiBold;  // QFont::DemiBold == Weight of 63
        case FontWeight700:
        case FontWeight800:
            return QFont::Bold;  // QFont::Bold == Weight of 75
        case FontWeight900:
            return QFont::Black;  // QFont::Black == Weight of 87
        case FontWeight300:
        case FontWeight400:
        case FontWeight500:
        default:
            return QFont::Normal;  // QFont::Normal == Weight of 50
        }
    }

    QFont font() const { return m_font; }
    float size() const { return m_size; }
    QString family() const { return m_font.family(); }
    bool bold() const { return m_bold; }
    bool italic() const { return m_font.italic(); }
    bool smallCaps() const { return m_font.capitalization() == QFont::SmallCaps; }
    int pixelSize() const { return m_font.pixelSize(); }

#ifndef NDEBUG
    String description() const;
#endif

    float m_size;
    bool m_bold;
    bool m_oblique;
    QFont m_font;
};

} // namespace WebCore

#endif // FontPlatformData_h
