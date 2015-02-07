

#ifndef CurrentTime_h
#define CurrentTime_h

namespace WTF {

    // Returns the current system (UTC) time in seconds, starting January 1, 1970.
    // Precision varies depending on a platform but usually is as good or better 
    // than a millisecond.
    double currentTime();

#if PLATFORM(ANDROID)
    uint32_t get_thread_msec();
#endif

} // namespace WTF

using WTF::currentTime;

#endif // CurrentTime_h

