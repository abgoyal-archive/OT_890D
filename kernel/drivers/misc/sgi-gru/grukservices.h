

#ifndef __GRU_KSERVICES_H_
#define __GRU_KSERVICES_H_



extern int gru_create_message_queue(void *p, unsigned int bytes);

extern int gru_send_message_gpa(unsigned long mq_gpa, void *mesg,
						unsigned int bytes);

/* Status values for gru_send_message() */
#define MQE_OK			0	/* message sent successfully */
#define MQE_CONGESTION		1	/* temporary congestion, try again */
#define MQE_QUEUE_FULL		2	/* queue is full */
#define MQE_UNEXPECTED_CB_ERR	3	/* unexpected CB error */
#define MQE_PAGE_OVERFLOW	10	/* BUG - queue overflowed a page */
#define MQE_BUG_NO_RESOURCES	11	/* BUG - could not alloc GRU cb/dsr */

extern void gru_free_message(void *mq, void *mesq);

extern void *gru_get_next_message(void *mq);


extern int gru_copy_gpa(unsigned long dest_gpa, unsigned long src_gpa,
							unsigned int bytes);

#endif 		/* __GRU_KSERVICES_H_ */
