

#include "cx18-driver.h"
#include "cx18-io.h"
#include "cx18-i2c.h"
#include "cx18-cards.h"
#include "cx18-audio.h"

#define CX18_AUDIO_ENABLE 0xc72014

int cx18_audio_set_io(struct cx18 *cx)
{
	struct v4l2_routing route;
	u32 audio_input;
	u32 val;
	int mux_input;
	int err;

	/* Determine which input to use */
	if (test_bit(CX18_F_I_RADIO_USER, &cx->i_flags)) {
		audio_input = cx->card->radio_input.audio_input;
		mux_input = cx->card->radio_input.muxer_input;
	} else {
		audio_input =
			cx->card->audio_inputs[cx->audio_input].audio_input;
		mux_input =
			cx->card->audio_inputs[cx->audio_input].muxer_input;
	}

	/* handle muxer chips */
	route.input = mux_input;
	route.output = 0;
	cx18_i2c_hw(cx, cx->card->hw_muxer, VIDIOC_INT_S_AUDIO_ROUTING, &route);

	route.input = audio_input;
	err = cx18_i2c_hw(cx, cx->card->hw_audio_ctrl,
			VIDIOC_INT_S_AUDIO_ROUTING, &route);
	if (err)
		return err;

	val = cx18_read_reg(cx, CX18_AUDIO_ENABLE) & ~0x30;
	val |= (audio_input > CX18_AV_AUDIO_SERIAL2) ? 0x20 :
					(audio_input << 4);
	cx18_write_reg(cx, val | 0xb00, CX18_AUDIO_ENABLE);
	cx18_vapi(cx, CX18_APU_RESETAI, 1, 0);
	return 0;
}

void cx18_audio_set_route(struct cx18 *cx, struct v4l2_routing *route)
{
	cx18_i2c_hw(cx, cx->card->hw_audio_ctrl,
			VIDIOC_INT_S_AUDIO_ROUTING, route);
}

void cx18_audio_set_audio_clock_freq(struct cx18 *cx, u8 freq)
{
	static u32 freqs[3] = { 44100, 48000, 32000 };

	/* The audio clock of the digitizer must match the codec sample
	   rate otherwise you get some very strange effects. */
	if (freq > 2)
		return;
	cx18_call_i2c_clients(cx, VIDIOC_INT_AUDIO_CLOCK_FREQ, &freqs[freq]);
}
