

#ifndef M5602_SENSOR_H_
#define M5602_SENSOR_H_

#include "m5602_bridge.h"

#define M5602_DEFAULT_FRAME_WIDTH  640
#define M5602_DEFAULT_FRAME_HEIGHT 480

#define M5602_MAX_CTRLS		(V4L2_CID_LASTP1 - V4L2_CID_BASE + 10)

/* Enumerates all supported sensors */
enum sensors {
	OV9650_SENSOR	= 1,
	S5K83A_SENSOR	= 2,
	S5K4AA_SENSOR	= 3,
	MT9M111_SENSOR	= 4,
	PO1030_SENSOR	= 5
};

/* Enumerates all possible instruction types */
enum instruction {
	BRIDGE,
	SENSOR,
	SENSOR_LONG
};

struct m5602_sensor {
	/* Defines the name of a sensor */
	char name[32];

	/* What i2c address the sensor is connected to */
	u8 i2c_slave_id;

	/* Width of each i2c register (in bytes) */
	u8 i2c_regW;

	/* Probes if the sensor is connected */
	int (*probe)(struct sd *sd);

	/* Performs a initialization sequence */
	int (*init)(struct sd *sd);

	/* Executed when the camera starts to send data */
	int (*start)(struct sd *sd);

	/* Performs a power down sequence */
	int (*power_down)(struct sd *sd);

	int nctrls;
	struct ctrl ctrls[M5602_MAX_CTRLS];

	char nmodes;
	struct v4l2_pix_format modes[];
};

#endif
