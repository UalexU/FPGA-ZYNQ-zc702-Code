#include "xiic.h"

XIic_Config XIic_ConfigTable[] __attribute__ ((section (".drvcfg_sec"))) = {

	{
		"xlnx,axi-iic-2.1", /* compatible */
		0x41600000, /* reg */
		0x0, /* xlnx,ten-bit-adr */
		0x1, /* xlnx,gpo-width */
		0x401e, /* interrupts */
		0xf8f01000 /* interrupt-parent */
	},
	 {
		 NULL
	}
};