


#include "MT6516_MPEG4_UTILITY.h"

///////////////////////////////////////////
//             mp4 utilities
//

//UINT32 MP4_base;

#define MAX_MP4_WARNING  16
UINT32 mp4_warning_line[MAX_MP4_WARNING];
void mp4_warning(UINT32 line)
{
	static UINT32 mp4_warning_index = 0;
	mp4_warning_line[mp4_warning_index&(MAX_MP4_WARNING - 1)] = line;
	mp4_warning_index++;
}


INT32 mp4_util_show_bits(UINT8 * data, INT32 bitcnt, INT32 num)
{
	INT32 tmp, out, tmp1;

	tmp = (bitcnt & 0x7) + num;

	if (tmp <= 8)
		out = (data[bitcnt >> 3] >> (8 - tmp)) & ((1 << num) - 1);
	else
	{
		out = data[bitcnt >> 3]&((1 << (8 - (bitcnt & 0x7))) - 1);

		tmp -= 8;
		bitcnt += (8 - (bitcnt & 0x7));

		while (tmp > 8)
		{
			out = (out << 8) + data[bitcnt >> 3];

			tmp -= 8;
			bitcnt += 8;
		}

		tmp1 = (data[bitcnt >> 3] >> (8 - tmp)) & ((1 << tmp) - 1);
		out = (out << tmp) + tmp1;
	}

	return out;
}


INT32 mp4_util_get_bits(UINT8 * data, INT32 *bitcnt, INT32 num)
{
	UINT32 ret;
	
	ret = mp4_util_show_bits(data,*bitcnt,num);
	(*bitcnt)+=num;

	return ret;
}


INT32 mp4_util_show_word(UINT8 * a)
{
	return ((a[0] << 24) + (a[1] << 16) + (a[2] << 8) + a[3]);
}


INT32 mp4_util_log2ceil(INT32 arg)
{
	INT32 j = 0, i = 1;

	while (arg > i)
	{
		i *= 2;
		j++;
	}

	if (j == 0)
		j = 1;

	return j;
}


INT32 mp4_util_user_data(UINT8 * data, INT32 * bitcnt, UINT32 max_parse_data_size)
{
	INT32 bit = 0;

	*bitcnt += 32;

	while (mp4_util_show_bits(data, *bitcnt + bit, 24) != 1)
	{
		bit += 8;
		if ((UINT32)bit > max_parse_data_size)
		{
			break;
		}   
	}

	*bitcnt += bit;

	return 0;
}


void mp4_putbits(UINT8 * in, INT32 * bitcnt, INT32 data, INT32 data_length)
{
	UINT8 *temp, *temp1;

	INT32 t, count, count2;
	t = *bitcnt;
	temp = in;

	if ((t & 0x7) == 0)
		temp[t >> 3] = 0;

	if ((t & 0x7) + data_length <= 8)
	{
		temp[(t >> 3)] |= (data << (8 - ((t & 0x7) + data_length)));
	}
	else
	{
		count = (t & 7) + data_length;
		
		temp1 = &temp[t >> 3];
		*temp1 |= (data >> (data_length - 8 + (t & 7)));
		count2 = count - 16;
		temp1++;
		
		while (count2 >= 0)
		{
			*temp1 = (data >> count2) & 0xFF;
			
			temp1++;
			count2 -= 8;
		}
		*temp1 = (data&((1 << (count % 8)) - 1)) << ((8 - count % 8));
	}

	*bitcnt += data_length;
}



