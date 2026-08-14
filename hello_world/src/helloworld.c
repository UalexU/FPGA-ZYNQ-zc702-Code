#include "xparameters.h"
#include "xgpiops.h"
#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"
#include <xil_types.h>

#define LED_PIN  8
#define BTN_PIN_SW14  12       /* placeholder - task 2 finds the real value */
#define BTN_PIN_SW13  14 

static XGpioPs Gpio;

/* Standard driver bring-up. Identical for every Xilinx driver: look up
   the hardware description, then initialise an instance from it.
   You would paste this in real life too. */
static int gpio_init(void)
{
    XGpioPs_Config *cfg = XGpioPs_LookupConfig(XPAR_XGPIOPS_0_BASEADDR);
    /* Vitis 2025.1 SDT flow: XGpioPs_LookupConfig(XPAR_XGPIOPS_0_BASEADDR) */
    if (cfg == NULL) {
        xil_printf("No GPIO in this BSP - did you tick GPIO MIO in Vivado?\r\n");
        return XST_FAILURE;
    }
    return XGpioPs_CfgInitialize(&Gpio, cfg, cfg->BaseAddr);
}

void configure_led(void){
            XGpioPs_SetDirectionPin(&Gpio, LED_PIN, 1);
            //XGpioPs_SetOutputEnablePin(&Gpio, LED_PIN, 1);
        }


void find_button(void){

    u32 prev[16]; 
    for (int i=8; i < 15; i++){
            XGpioPs_SetDirectionPin(&Gpio, i, 0);
            prev[i] = XGpioPs_ReadPin(&Gpio, i);
    }

    xil_printf("Press BTN SW13 to read the pin");

    while(1){
        for(int p = 8; p <= 15; p++){
            u32 now = XGpioPs_ReadPin(&Gpio, p);
            if (now != prev[p]){
                xil_printf("MIO %2d -> %lu\r\n", p, now);
                prev[p] = now;
            }
        }
        usleep(2000);

    }


}
int main(void)
{
    init_platform();
    xil_printf("\r\nM01 starting\r\n");
    if (gpio_init() != XST_SUCCESS) return -1;
    configure_led(); 
    // find_button(); 
    /* --- your code goes here --- */
    XGpioPs_SetDirectionPin(&Gpio, BTN_PIN_SW14, 0);
    xil_printf("Led turns on when press BTN SW14\r\n");

    while (1) {
    XGpioPs_WritePin(&Gpio, LED_PIN, XGpioPs_ReadPin(&Gpio, BTN_PIN_SW14));
    usleep(10000);
}
    return 0;
}