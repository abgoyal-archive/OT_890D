

#define CX18_DMA_UNMAPPED	((u32) -1)

/* cx18_buffer utility functions */

static inline void cx18_buf_sync_for_cpu(struct cx18_stream *s,
	struct cx18_buffer *buf)
{
	pci_dma_sync_single_for_cpu(s->cx->dev, buf->dma_handle,
				s->buf_size, s->dma);
}

static inline void cx18_buf_sync_for_device(struct cx18_stream *s,
	struct cx18_buffer *buf)
{
	pci_dma_sync_single_for_device(s->cx->dev, buf->dma_handle,
				s->buf_size, s->dma);
}

void cx18_buf_swap(struct cx18_buffer *buf);

/* cx18_queue utility functions */
struct cx18_queue *_cx18_enqueue(struct cx18_stream *s, struct cx18_buffer *buf,
				 struct cx18_queue *q, int to_front);

static inline
struct cx18_queue *cx18_enqueue(struct cx18_stream *s, struct cx18_buffer *buf,
				struct cx18_queue *q)
{
	return _cx18_enqueue(s, buf, q, 0); /* FIFO */
}

static inline
struct cx18_queue *cx18_push(struct cx18_stream *s, struct cx18_buffer *buf,
			     struct cx18_queue *q)
{
	return _cx18_enqueue(s, buf, q, 1); /* LIFO */
}

void cx18_queue_init(struct cx18_queue *q);
struct cx18_buffer *cx18_dequeue(struct cx18_stream *s, struct cx18_queue *q);
struct cx18_buffer *cx18_queue_get_buf(struct cx18_stream *s, u32 id,
	u32 bytesused);
void cx18_flush_queues(struct cx18_stream *s);

/* cx18_stream utility functions */
int cx18_stream_alloc(struct cx18_stream *s);
void cx18_stream_free(struct cx18_stream *s);
