
/* system header files */
#include <linux/init.h>
extern __init int board_init(void);
late_initcall(board_init);
