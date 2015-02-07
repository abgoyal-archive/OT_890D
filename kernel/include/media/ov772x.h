

#ifndef __OV772X_H__
#define __OV772X_H__

#include <media/soc_camera.h>

struct ov772x_camera_info {
	unsigned long          buswidth;
	struct soc_camera_link link;
};

#endif /* __OV772X_H__ */
