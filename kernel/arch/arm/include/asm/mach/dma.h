

struct dma_struct;
typedef struct dma_struct dma_t;

struct dma_ops {
	int	(*request)(dmach_t, dma_t *);		/* optional */
	void	(*free)(dmach_t, dma_t *);		/* optional */
	void	(*enable)(dmach_t, dma_t *);		/* mandatory */
	void 	(*disable)(dmach_t, dma_t *);		/* mandatory */
	int	(*residue)(dmach_t, dma_t *);		/* optional */
	int	(*setspeed)(dmach_t, dma_t *, int);	/* optional */
	char	*type;
};

struct dma_struct {
	void		*addr;		/* single DMA address		*/
	unsigned long	count;		/* single DMA size		*/
	struct scatterlist buf;		/* single DMA			*/
	int		sgcount;	/* number of DMA SG		*/
	struct scatterlist *sg;		/* DMA Scatter-Gather List	*/

	unsigned int	active:1;	/* Transfer active		*/
	unsigned int	invalid:1;	/* Address/Count changed	*/

	dmamode_t	dma_mode;	/* DMA mode			*/
	int		speed;		/* DMA speed			*/

	unsigned int	lock;		/* Device is allocated		*/
	const char	*device_id;	/* Device name			*/

	unsigned int	dma_base;	/* Controller base address	*/
	int		dma_irq;	/* Controller IRQ		*/
	struct scatterlist cur_sg;	/* Current controller buffer	*/
	unsigned int	state;

	struct dma_ops	*d_ops;
};

extern void arch_dma_init(dma_t *dma);

extern void isa_init_dma(dma_t *dma);
