
#ifndef TOUCHPANEL_H
#define TOUCHPANEL_H

#define TPD_TYPE_CAPACITIVE
//#define TPD_TYPE_RESISTIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE         MT6516_POWER_V3GRX
#define TPD_I2C_NUMBER           2
#define TPD_WAKEUP_TRIAL         15
#define TPD_WAKEUP_DELAY         100

#define TPD_DELAY                (2*HZ/100)
#define TPD_RES_X                320
#define TPD_RES_Y                480

/* to turn on calibration, unmark these two lines */
//#define TPD_CALIBRATION_MATRIX  {640, 0, 0, 0, -1000, 2048000, 0, 0};
#define TPD_CALIBRATION_MATRIX  {680, 0, -81920, 0, -1060, 2129920, 0, 0};
#define TPD_HAVE_CALIBRATION
#define TPD_WARP_START          {10,0,300,470};
#define TPD_WARP_END            {5,0,314,475};


//#define TPD_CUSTOM_CALIBRATION

#define TPD_HAVE_BUTTON
#define TPD_BUTTON_HEIGHT	480
#define TPD_KEY_COUNT           4
#define TPD_KEYS                {KEY_BACK, KEY_MENU, KEY_HOME, KEY_SEARCH}
#define TPD_KEYS_DIM            {{40,505,80,50},{120,505,80,50},{200,505,80,50},{280,505,80,50}}




#define TPD_HAVE_TREMBLE_ELIMINATION
#define TPD_CUSTOM_TREMBLE_TOLERANCE
#define TPD_FAT_TOUCH 120


#endif

