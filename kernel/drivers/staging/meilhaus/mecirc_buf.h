


#ifndef _MECIRC_BUF_H_
#define _MECIRC_BUF_H_

# ifdef __KERNEL__

#  ifdef BOSCH

typedef struct me_circ_buf {
	unsigned int mask;
//      unsigned int count;
	uint32_t *buf;
	int volatile head;
	int volatile tail;
} me_circ_buf_t;

static int inline me_circ_buf_values(me_circ_buf_t * buf)
{
//      return ((buf->head - buf->tail) & (buf->count - 1));
	return ((buf->head - buf->tail) & (buf->mask));
}

static int inline me_circ_buf_space(me_circ_buf_t * buf)
{
//      return ((buf->tail - (buf->head + 1)) & (buf->count - 1));
	return ((buf->tail - (buf->head + 1)) & (buf->mask));
}

static int inline me_circ_buf_values_to_end(me_circ_buf_t * buf)
{
	int end;
	int n;
//      end = buf->count - buf->tail;
//      n = (buf->head + end) & (buf->count - 1);
	end = buf->mask + 1 - buf->tail;
	n = (buf->head + end) & (buf->mask);
	return (n < end) ? n : end;
}

static int inline me_circ_buf_space_to_end(me_circ_buf_t * buf)
{
	int end;
	int n;

//      end = buf->count - 1 - buf->head;
//      n = (end + buf->tail) & (buf->count - 1);
	end = buf->mask - buf->head;
	n = (end + buf->tail) & (buf->mask);
	return (n <= end) ? n : (end + 1);
}

#define _CBUFF_32b_t

#  else	//~BOSCH
/// @note buf->mask = buf->count-1 = ME4600_AI_CIRC_BUF_COUNT-1

#   ifdef _CBUFF_32b_t
	//32 bit
typedef struct me_circ_buf_32b {
	int volatile head;
	int volatile tail;
	unsigned int mask;	//buffor size-1 must be 2^n-1 to work
	uint32_t *buf;
} me_circ_buf_t;
#   else
	//16 bit
typedef struct me_circ_buf_16b {
	int volatile head;
	int volatile tail;
	unsigned int mask;	//buffor size-1 must be 2^n-1 to work
	uint16_t *buf;
} me_circ_buf_t;
#   endif //_CBUFF_32b_t

/** How many values is in buffer */
static int inline me_circ_buf_values(me_circ_buf_t * buf)
{
	return ((buf->head - buf->tail) & (buf->mask));
}

/** How many space left */
static int inline me_circ_buf_space(me_circ_buf_t * buf)
{
	return ((buf->tail - (buf->head + 1)) & (buf->mask));
}

/** How many values can be read from buffor in one chunck. */
static int inline me_circ_buf_values_to_end(me_circ_buf_t * buf)
{
	return (buf->tail <=
		buf->head) ? (buf->head - buf->tail) : (buf->mask - buf->tail +
							1);
}

/** How many values can be write to buffer in one chunck. */
static int inline me_circ_buf_space_to_end(me_circ_buf_t * buf)
{
	return (buf->tail <=
		buf->head) ? (buf->mask - buf->head + 1) : (buf->tail -
							    buf->head - 1);
}

#  endif //BOSCH
# endif	//__KERNEL__
#endif //_MECIRC_BUF_H_
