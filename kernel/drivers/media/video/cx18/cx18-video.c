

#include "cx18-driver.h"
#include "cx18-video.h"
#include "cx18-av-core.h"
#include "cx18-cards.h"

void cx18_video_set_io(struct cx18 *cx)
{
	struct v4l2_routing route;
	int inp = cx->active_input;
	u32 type;

	route.input = cx->card->video_inputs[inp].video_input;
	route.output = 0;
	cx18_av_cmd(cx, VIDIOC_INT_S_VIDEO_ROUTING, &route);

	type = cx->card->video_inputs[inp].video_type;

	if (type == CX18_CARD_INPUT_VID_TUNER)
		route.input = 0;  /* Tuner */
	else if (type < CX18_CARD_INPUT_COMPOSITE1)
		route.input = 2;  /* S-Video */
	else
		route.input = 1;  /* Composite */
}
