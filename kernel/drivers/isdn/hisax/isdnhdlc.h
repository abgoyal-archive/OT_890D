

#ifndef __ISDNHDLC_H__
#define __ISDNHDLC_H__

struct isdnhdlc_vars {
	int bit_shift;
	int hdlc_bits1;
	int data_bits;
	int ffbit_shift; 	// encoding only
	int state;
	int dstpos;

	unsigned short crc;

	unsigned char cbin;
	unsigned char shift_reg;
	unsigned char ffvalue;

	unsigned int data_received:1; 	// set if transferring data
	unsigned int dchannel:1; 	// set if D channel (send idle instead of flags)
	unsigned int do_adapt56:1; 	// set if 56K adaptation
	unsigned int do_closing:1; 	// set if in closing phase (need to send CRC + flag
};


#define HDLC_FRAMING_ERROR     1
#define HDLC_CRC_ERROR         2
#define HDLC_LENGTH_ERROR      3

extern void isdnhdlc_rcv_init (struct isdnhdlc_vars *hdlc, int do_adapt56);

extern int isdnhdlc_decode (struct isdnhdlc_vars *hdlc, const unsigned char *src, int slen,int *count,
	                    unsigned char *dst, int dsize);

extern void isdnhdlc_out_init (struct isdnhdlc_vars *hdlc,int is_d_channel,int do_adapt56);

extern int isdnhdlc_encode (struct isdnhdlc_vars *hdlc,const unsigned char *src,unsigned short slen,int *count,
	                    unsigned char *dst,int dsize);

#endif /* __ISDNHDLC_H__ */
