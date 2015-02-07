
#ifndef TOUCHPANEL_H
#define TOUCHPANEL_H

/* turn on only one of them */
//#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE

#define TPD_DELAY               (2*HZ/100)
#define TPD_RES_X               240
#define TPD_RES_Y               320
#define TPD_PRESSURE_MAX        8000
#define TPD_PRESSURE_MIN        200
#define TPD_PRESSURE_NICE       6000
#define TPD_COUNT_TOLERANCE     3
//mopdified by luochao for pr144968 begin to follow BabydDul coefficient
#define TPD_CALIBRATION_MATRIX {1103,-3,-78564,-6,-1654,1655932,0,3};
//#define TPD_CALIBRATION_MATRIX  {960,0,0,0,-1520,1556480};
//mopdified by luochao for pr144968 end
//#define TP_CALIBRATION_MATRIX   {1119,-6,-96399,-3,-1974,1952300}; // old

#define TPD_HAVE_TREMBLE_ELIMINATION

#define TPD_HAVE_CALIBRATION

#define TPD_HAVE_DRIFT_ELIMINATION

//#define TPD_HAVE_BUTTON
//#define TPD_BUTTON_HEIGHT       400
//#define TPD_LEFT_KEY            KEY_MENU
//#define TPD_CENTER_KEY          KEY_HOME
//#define TPD_RIGHT_KEY           KEY_BACK


#define TPD_HAVE_BUTTON
/*=================================sunxiaoye modify Touch key sensibility begin*====================================================*/
//#define TPD_BUTTON_HEIGHT	330
//#define TPD_KEY_COUNT           6
//#define TPD_KEYS                {KEY_MENU, KEY_SEARCH, KEY_BACK, KEY_SEND, KEY_HOME, KEY_END}
//#define TPD_KEYS_DIM            {{40,345,80,50},{120,345,80,50},{200,345,80,50},{40,410,80,80},{120,410,80,80},{200,410,80,80}}
#define TPD_BUTTON_HEIGHT	345
#define TPD_KEY_COUNT           3
#define TPD_KEYS                {KEY_MENU, KEY_SEARCH, KEY_BACK}
#define TPD_KEYS_DIM            {{40,360,80,50},{120,360,80,50},{200,360,80,50}}
/*=================================sunxiaoye modify Touch key sensibility end*====================================================*/


#define TPD_HAVE_ADV_DRIFT_ELIMINATION
#define TPD_ADE_P1 1800
#define TPD_ADE_P2 2000

//#define TPD_HAVE_TRACK_EXTENSION


#endif
