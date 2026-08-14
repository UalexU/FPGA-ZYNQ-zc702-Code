#include "xil_io.h"
#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"

#define SLCR_BASE       0xF8000000
#define SLCR_LOCK       (SLCR_BASE + 0x004)   /* write 0x767B */
#define SLCR_UNLOCK     (SLCR_BASE + 0x008)   /* write 0xDF0D */
#define APER_CLK_CTRL   (SLCR_BASE + 0x12C)   /* peripheral clock gates */

/* GPIO registers from M02 */
#define GPIO_BASE  0xE000A000
#define DATA_0     (GPIO_BASE + 0x040)
#define DIRM_0     (GPIO_BASE + 0x204)
#define OEN_0      (GPIO_BASE + 0x208)
#define LED_PIN  8
#define BTN_PIN_SW14  12       
#define BTN_PIN_SW13  14 

int main(void)
{
    init_platform();

    // Setting my output pins. 
    Xil_Out32(DIRM_0, Xil_In32(DIRM_0) | (1u << LED_PIN)); // This makes the offset bit that sets direction On
    Xil_Out32(OEN_0,  Xil_In32(OEN_0)  | (1u << LED_PIN)); // THis makes the offsett bit that enables ON

    xil_printf("\r\nM03 starting\r\n");
    xil_printf("Aper Clk Ctrl: 0x%08lX\r\n", Xil_In32(APER_CLK_CTRL));
    Xil_Out32(SLCR_UNLOCK, 0xDF0D);

while (1) {

    xil_printf("Clock is ON - LED SHOULD BE ON");
    for (int i = 0; i < 5; i++){
        Xil_Out32(DATA_0, Xil_In32(DATA_0) | (1u << LED_PIN));
        usleep(200000);
        Xil_Out32(DATA_0, Xil_In32(DATA_0) & ~(1u << LED_PIN));
        usleep(200000);

    }

    // Now I will turn off the clock for GPIO

    Xil_Out32(APER_CLK_CTRL, Xil_In32(APER_CLK_CTRL) & ~(1u << 22)); // OFF
    // My LED should not change because the clock does not move. 
    for (int i = 0; i < 5; i++){
        Xil_Out32(DATA_0, Xil_In32(DATA_0) | (1u << LED_PIN));
        usleep(200000);
        Xil_Out32(DATA_0, Xil_In32(DATA_0) & ~(1u << LED_PIN));
        usleep(200000);

    }

    // Restored clock, LED should turn on again
    Xil_Out32(APER_CLK_CTRL, Xil_In32(APER_CLK_CTRL) |  (1u << 22)); // ON Clokc
    Xil_Out32(DATA_0, Xil_In32(DATA_0) | (1u << LED_PIN)); // ON LED
    usleep(200000);
}
    
    return 0;
}