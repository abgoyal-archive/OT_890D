

#ifndef DateConversion_h
#define DateConversion_h

namespace WTF {
    struct GregorianDateTime;
}

namespace JSC {

class UString;

double parseDate(const UString&);
UString formatDate(const WTF::GregorianDateTime&);
UString formatDateUTCVariant(const WTF::GregorianDateTime&);
UString formatTime(const WTF::GregorianDateTime&, bool inputIsUTC);

} // namespace JSC

#endif // DateConversion_h
