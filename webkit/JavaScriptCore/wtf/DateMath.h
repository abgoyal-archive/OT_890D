

#ifndef DateMath_h
#define DateMath_h

#include <time.h>
#include <string.h>
#include <wtf/Noncopyable.h>

namespace WTF {

struct GregorianDateTime;

void initializeDates();
void msToGregorianDateTime(double, bool outputIsUTC, GregorianDateTime&);
double gregorianDateTimeToMS(const GregorianDateTime&, double, bool inputIsUTC);
double getUTCOffset();
int equivalentYearForDST(int year);
double getCurrentUTCTime();
double getCurrentUTCTimeWithMicroseconds();
void getLocalTime(const time_t*, tm*);

// Not really math related, but this is currently the only shared place to put these.  
double parseDateFromNullTerminatedCharacters(const char*);
double timeClip(double);

const char * const weekdayName[7] = { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
const char * const monthName[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

const double hoursPerDay = 24.0;
const double minutesPerHour = 60.0;
const double secondsPerHour = 60.0 * 60.0;
const double secondsPerMinute = 60.0;
const double msPerSecond = 1000.0;
const double msPerMinute = 60.0 * 1000.0;
const double msPerHour = 60.0 * 60.0 * 1000.0;
const double msPerDay = 24.0 * 60.0 * 60.0 * 1000.0;

// Intentionally overridding the default tm of the system
// Tee members of tm differ on various operating systems.
struct GregorianDateTime : Noncopyable {
    GregorianDateTime()
        : second(0)
        , minute(0)
        , hour(0)
        , weekDay(0)
        , monthDay(0)
        , yearDay(0)
        , month(0)
        , year(0)
        , isDST(0)
        , utcOffset(0)
        , timeZone(0)
    {
    }

    ~GregorianDateTime()
    {
        delete [] timeZone;
    }

    GregorianDateTime(const tm& inTm)
        : second(inTm.tm_sec)
        , minute(inTm.tm_min)
        , hour(inTm.tm_hour)
        , weekDay(inTm.tm_wday)
        , monthDay(inTm.tm_mday)
        , yearDay(inTm.tm_yday)
        , month(inTm.tm_mon)
        , year(inTm.tm_year)
        , isDST(inTm.tm_isdst)
    {
#if HAVE(TM_GMTOFF)
        utcOffset = static_cast<int>(inTm.tm_gmtoff);
#else
        utcOffset = static_cast<int>(getUTCOffset() / msPerSecond + (isDST ? secondsPerHour : 0));
#endif

#if HAVE(TM_ZONE)
        int inZoneSize = strlen(inTm.tm_zone) + 1;
        timeZone = new char[inZoneSize];
        strncpy(timeZone, inTm.tm_zone, inZoneSize);
#else
        timeZone = 0;
#endif
    }

    operator tm() const
    {
        tm ret;
        memset(&ret, 0, sizeof(ret));

        ret.tm_sec   =  second;
        ret.tm_min   =  minute;
        ret.tm_hour  =  hour;
        ret.tm_wday  =  weekDay;
        ret.tm_mday  =  monthDay;
        ret.tm_yday  =  yearDay;
        ret.tm_mon   =  month;
        ret.tm_year  =  year;
        ret.tm_isdst =  isDST;

#if HAVE(TM_GMTOFF)
        ret.tm_gmtoff = static_cast<long>(utcOffset);
#endif
#if HAVE(TM_ZONE)
        ret.tm_zone = timeZone;
#endif

        return ret;
    }

    void copyFrom(const GregorianDateTime& rhs)
    {
        second = rhs.second;
        minute = rhs.minute;
        hour = rhs.hour;
        weekDay = rhs.weekDay;
        monthDay = rhs.monthDay;
        yearDay = rhs.yearDay;
        month = rhs.month;
        year = rhs.year;
        isDST = rhs.isDST;
        utcOffset = rhs.utcOffset;
        if (rhs.timeZone) {
            int inZoneSize = strlen(rhs.timeZone) + 1;
            timeZone = new char[inZoneSize];
            strncpy(timeZone, rhs.timeZone, inZoneSize);
        } else
            timeZone = 0;
    }

    int second;
    int minute;
    int hour;
    int weekDay;
    int monthDay;
    int yearDay;
    int month;
    int year;
    int isDST;
    int utcOffset;
    char* timeZone;
};

static inline int gmtoffset(const GregorianDateTime& t)
{
    return t.utcOffset;
}

} // namespace WTF

#endif // DateMath_h
