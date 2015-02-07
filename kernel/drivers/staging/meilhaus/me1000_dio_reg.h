


#ifndef _ME1000_DIO_REG_H_
# define _ME1000_DIO_REG_H_

# ifdef __KERNEL__

# define ME1000_DIO_NUMBER_CHANNELS	32				/**< The number of channels per DIO port. */
# define ME1000_DIO_NUMBER_PORTS	4				/**< The number of ports per ME-1000. */

// # define ME1000_PORT_A                               0x0000                  /**< Port A base register offset. */
// # define ME1000_PORT_B                               0x0004                  /**< Port B base register offset. */
// # define ME1000_PORT_C                               0x0008                  /**< Port C base register offset. */
// # define ME1000_PORT_D                               0x000C                  /**< Port D base register offset. */
# define ME1000_PORT				0x0000			/**< Base for port's register. */
# define ME1000_PORT_STEP			4				/**< Distance between port's register. */

# define ME1000_PORT_MODE			0x0010			/**< Configuration register to switch the port direction. */
// # define ME1000_PORT_MODE_OUTPUT_A   (1 << 0)                /**< If set, port A is in output, otherwise in input mode. */
// # define ME1000_PORT_MODE_OUTPUT_B   (1 << 1)                /**< If set, port B is in output, otherwise in input mode. */
// # define ME1000_PORT_MODE_OUTPUT_C   (1 << 2)                /**< If set, port C is in output, otherwise in input mode. */
// # define ME1000_PORT_MODE_OUTPUT_D   (1 << 3)                /**< If set, port D is in output, otherwise in input mode. */

# endif	//__KERNEL__
#endif //_ME1000_DIO_REG_H_
