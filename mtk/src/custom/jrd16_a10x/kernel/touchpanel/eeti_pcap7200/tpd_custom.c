
#include "tpd.h"

#ifdef TPD_CUSTOM_CALIBRATION
void tpd_calibrate(int *x, int *y) {
    *y      = TPD_RES_Y-(((*y)*TPD_RES_Y)>>11);
    *x      = ((*x)*TPD_RES_X)>>11;
}
#endif

#ifdef TPD_CUSTOM_TREMBLE_TOLERANCE
int tpd_trembling_tolerance(int t, int p) {
    if(p>100) return 900;
    if(p>90) return 400;
    if(t>5) return 200;
    if(p>80) return 64;
    if(p>70) return 36;
    return 26;
}
#endif
