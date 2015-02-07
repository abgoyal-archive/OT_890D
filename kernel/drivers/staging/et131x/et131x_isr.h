

#ifndef __ET131X_ISR_H__
#define __ET131X_ISR_H__

irqreturn_t et131x_isr(int irq, void *dev_id);
void et131x_isr_handler(struct work_struct *work);

#endif /* __ET131X_ISR_H__ */
