

#ifndef WTF_dtoa_h
#define WTF_dtoa_h

namespace WTF {
    class Mutex;
}

namespace WTF {

    extern WTF::Mutex* s_dtoaP5Mutex;

    double strtod(const char* s00, char** se);
    void dtoa(char* result, double d, int ndigits, int* decpt, int* sign, char** rve);

} // namespace WTF

#endif // WTF_dtoa_h
