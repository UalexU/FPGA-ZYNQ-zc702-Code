#include "xil_io.h"       /* Xil_In32 / Xil_Out32 */
#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"

#define GPIO_BASE  0xE000A000     /* PS GPIO controller */
#define LED_BIT    8

#define MASK_DATA_0_LSW  (GPIO_BASE + 0x000)   /* [31:16] mask, [15:0] data */
#define DATA_0           (GPIO_BASE + 0x040)   
#define DATA_0_RO        (GPIO_BASE + 0x060)   /* what is on the PIN        */
#define DIRM_0           (GPIO_BASE + 0x204)
#define OEN_0            (GPIO_BASE + 0x208)


#define BTN_PIN_SW14  12       /* placeholder - task 2 finds the real value */
#define BTN_PIN_SW13  14 


int main(void)
{
    init_platform();
    xil_printf("\r\nM02 starting\r\n");
    
    // int value = read(DIRM_0);
    // value = (1 << LED_BIT);
    // write(DIRM_0, value);
    
    Xil_Out32(DIRM_0, Xil_In32(DIRM_0) |  (1u << LED_BIT));   /* output */
    Xil_Out32(OEN_0, Xil_In32(OEN_0) | (1u << LED_BIT)); /*enable*/
    Xil_Out32(DIRM_0, Xil_In32(DIRM_0) & ~ (1u << BTN_PIN_SW13)); 

    while (1) {
        u32 btn = (Xil_In32(DATA_0_RO) >> BTN_PIN_SW13) & 1u;
        u32 cur = Xil_In32(DATA_0);
        if (btn) cur |=  (1u << LED_BIT);
        else     cur &= ~(1u << LED_BIT);
        Xil_Out32(DATA_0, cur);
        usleep(10000);
        /* mask = ~(1<<8) = 0xFEFF, in the upper half.  Data in the lower half. */
        Xil_Out32(MASK_DATA_0_LSW, 0xFEFF0000u | (btn << LED_BIT));
}
    return 0;
}