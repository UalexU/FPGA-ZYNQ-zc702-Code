/*
 * XSpi (AXI Quad SPI, PL core) -- MAX31865 RTD read.
 * 
 * Configures the MAX31865, then reads all 8 registers (00h..07h) in
 * one multibyte transfer and decodes the RTD resistance.
 */
#include "xparameters.h"
#include "xspi.h"
#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"
#include <xspi.h>

static XSpi Spi;
/* 1 address byte + 8 data bytes = the whole register map. */
#define BufferOut 9
#define BufferIn 9

/* SPI mode 3 (CPOL=1, CPHA=1) with manual chip select.
 * Drop XSP_CLK_ACTIVE_LOW_OPTION for mode 1; the MAX31865 auto-detects
 * clock polarity, so only CPHA=1 is actually required. */
#define SPI_OPTIONS (XSP_MASTER_OPTION | \
                     XSP_CLK_ACTIVE_LOW_OPTION | \
                     XSP_CLK_PHASE_1_OPTION | \
                     XSP_MANUAL_SSELECT_OPTION)

/* Was 1 while automatic slave select starved the tCC setup time and the
 * data arrived one bit late. Manual slave select fixed the root cause,
 * so the data is now correctly aligned and shifting it would BREAK it.
 * Only set this back to 1 if you return to automatic slave select. */
#define APPLY_BIT_SHIFT  0

/* MAX31865 register addresses (Table 1).
 * Write addresses are the read address with bit 7 set. */
#define CONFIG_READ    0x00
#define CONFIG_WRITE   0x80
#define RTD_MSB_READ   0x01

/* Configuration register bits (Table 2). */
#define CFG_VBIAS_ON     0x80   /* D7: bias voltage on */
#define CFG_AUTO_CONV    0x40   /* D6: continuous conversion */
#define CFG_ONE_SHOT     0x20   /* D5: single conversion (self-clearing) */
#define CFG_3WIRE        0x10   /* D4: 1 = 3-wire, 0 = 2-wire or 4-wire */
#define CFG_FAULT_CLEAR  0x02   /* D1: clear fault status (self-clearing) */
#define CFG_50HZ         0x01   /* D0: 1 = 50Hz notch, 0 = 60Hz */

static int spi_init(void)
{
    XSpi_Config *cfg = XSpi_LookupConfig(XPAR_XSPI_0_BASEADDR);
    if (cfg == NULL) {
        return XST_FAILURE;
    }
    return XSpi_CfgInitialize(&Spi, cfg, cfg->BaseAddress);
}

int adc_to_rrtd(int adc){
    int RRTD = (adc*400)/32768;
    xil_printf("\r\nRTD Resistance is: %d\r\n", RRTD);
    return RRTD;
}

int adc_to_celcius(int adc){
    int celcius = (adc/32)-256;
    return celcius;
}

/* Linear approximation from page 11: T = (code / 32) - 256.
 * Returns TENTHS of a degree C, because xil_printf has no %f and
 * plain integer division would throw away the decimal. */
int adc_to_celcius_x10(int adc){
    return ((adc * 10) / 32) - 2560;
}

int rtd_to_adc(int rtd_1, int rtd_2){
    /* Read the fault flag BEFORE shifting: it lives in D0 of the LSB
     * byte, and the shift below discards exactly that bit. */
    int fault = rtd_2 & 1;

    rtd_1 = rtd_1 << 7;
    rtd_2 = rtd_2 >> 1;

    if (fault) {
        xil_printf("\r\nRTD fault bit set -- check register 07h\r\n");
    }
    return rtd_1 | rtd_2;

}

/* Shift the whole receive buffer left by one bit.
 *
 * Kept for reference. This compensated for data arriving one bit late
 * under automatic slave select. With manual slave select the data is
 * already aligned, so this is disabled via APPLY_BIT_SHIFT.
 *
 * Byte positions are unchanged, so rx[1] is still 00h, rx[2] is 01h,
 * and so on after the shift. */
void shift_buffer_left(u8 *buf, int len)
{
    for(int i = 0; i < len - 1; i++){
        buf[i] = (buf[i] << 1) | (buf[i+1] >> 7);
    }
    buf[len-1] = buf[len-1] << 1;   /* no next byte to pull a bit from */
}

int main(void)
{
    int status;

    init_platform();
    xil_printf("\r\nM13 starting (MAX31865)\r\n");

    status = spi_init();
    if (status != XST_SUCCESS) {
        xil_printf("SPI init failed (status=%d)\r\n", status);
        return XST_FAILURE;
    }

    /* Set the SPI device as a master with Mode 1 settings:
    * - XSP_MASTER_OPTION: Enables Master mode.
    * - XSP_CLK_PHASE_1_OPTION: Sets Clock Phase (CPHA) to 1.
    *   The MAX31865 requires CPHA=1 but auto-detects clock polarity
    *   at the CS falling edge, so CPOL does not matter (page 17).
    */
    /* XSP_MANUAL_SSELECT_OPTION: drive CS from the slave select register
     * instead of letting the core sequence it per transfer. The MAX31865
     * needs CS low at least 400ns before the first SCLK edge (tCC, page 4)
     * and samples SCLK at the CS falling edge to detect clock polarity
     * (page 17). Automatic mode starts the transfer on the first SCK edge,
     * which leaves no setup time for either. */
    status = XSpi_SetOptions(&Spi, SPI_OPTIONS);
    xil_printf("SetOptions status = %d\r\n", status);

    /* Read the options back to confirm the hardware actually took them. */
    xil_printf("Options wanted   = %08X\r\n", (unsigned int)SPI_OPTIONS);
    xil_printf("Options readback = %08X\r\n",
               (unsigned int)XSpi_GetOptions(&Spi));

    status = XSpi_SetSlaveSelect(&Spi, 0x1);
    xil_printf("SetSlaveSelect status = %d\r\n", status);
    /* XSpi requires an explicit Start -- without this the core stays
     * inhibited and Transfer silently does nothing. */
    status = XSpi_Start(&Spi);
    xil_printf("Start status = %d\r\n", status);

    /* THE KEY FIX: force polled (blocking) operation. Without this,
     * Transfer() can return almost immediately having only started
     * an interrupt-driven transfer, with nothing actually servicing
     * the completion interrupt -- which is why rx[] stayed at zero
     * every time even though status kept reporting success. */
    XSpi_IntrGlobalDisable(&Spi);

    /* Configure the MAX31865 before reading it. Out of reset the
     * config register is 00h: bias off and ADC off, so the RTD
     * registers never fill in. Bias on + auto conversion + clear
     * any stale faults. Add CFG_3WIRE here for a 3-wire RTD. */
    u8 cfg_tx[2] = { CONFIG_WRITE, CFG_VBIAS_ON | CFG_AUTO_CONV | CFG_FAULT_CLEAR };
    u8 cfg_rx[2] = { 0 };

    status = XSpi_Transfer(&Spi, cfg_tx, cfg_rx, 2);
    xil_printf("Config write status = %d (wrote %02X)\r\n", status, cfg_tx[1]);

    /* Continuous conversion mode: the first conversion after enabling
     * takes the single-conversion time, ~52ms at 60Hz (page 13). Wait
     * that out so the first read is not stale. */
    usleep(65000);

    int counter = 0;

    /* One address byte (00h) then 8 dummy bytes. The chip
     * auto-increments, so rx[1]..rx[8] hold registers 00h..07h. */
    u8 tx[BufferOut] = {CONFIG_READ};
    u8 rx[BufferIn] = {0};

    status = XSpi_Transfer(&Spi, tx, rx, BufferIn);
    XSpi_SetSlaveSelect(&Spi, 0);
    if (counter < 1) {
        xil_printf("Transfer status = %d\r\n", status);

        for(int count = 0; count < BufferIn; count++){
            if(count == 0){
                xil_printf("sent: %02X", tx[count]);
            }
            else{
                xil_printf(" %02X\r", tx[count]);
            }
        }

        for(int count = 0; count < BufferIn; count++){
            if(count == 0){
                xil_printf("\n\rgot:  %02X", rx[count]);
            }
            else{
                xil_printf(" %02X\r", rx[count]);
            }
        }

#if APPLY_BIT_SHIFT
        /* Realign the one-bit offset, then show the corrected bytes. */
        shift_buffer_left(rx, BufferIn);

        for(int count = 0; count < BufferIn; count++){
            if(count == 0){
                xil_printf("\n\rshifted: %02X", rx[count]);
            }
            else{
                xil_printf(" %02X\r", rx[count]);
            }
        }
#endif
    }

    counter++;

    /* Labeled dump so each byte can be checked against Table 1.
     * Sanity check: [4][5] must read FF FF and [6][7] must read 00 00.
     * Those are power-on constants, so if they are wrong the alignment
     * is wrong and nothing below can be trusted. */
    xil_printf("\n\r--- register map ---\n\r");
    xil_printf("  [0] discarded (SDO high-Z during address): %02X\n\r", rx[0]);
    xil_printf("  [1] 00h Configuration          : %02X\n\r", rx[1]);
    xil_printf("  [2] 01h RTD MSBs               : %02X\n\r", rx[2]);
    xil_printf("  [3] 02h RTD LSBs               : %02X\n\r", rx[3]);
    xil_printf("  [4] 03h High Fault Thresh MSB  : %02X\n\r", rx[4]);
    xil_printf("  [5] 04h High Fault Thresh LSB  : %02X\n\r", rx[5]);
    xil_printf("  [6] 05h Low Fault Thresh MSB   : %02X\n\r", rx[6]);
    xil_printf("  [7] 06h Low Fault Thresh LSB   : %02X\n\r", rx[7]);
    xil_printf("  [8] 07h Fault Status           : %02X\n\r", rx[8]);

    /* rx[1] is 00h, so the RTD pair is rx[2] (MSB) and rx[3] (LSB). */
    int adc = rtd_to_adc(rx[2], rx[3]);
    int rtd = adc_to_rrtd(adc);
    xil_printf("\n\rThe rtd to adc: %d\n\r", adc);
    xil_printf("\n\rRTD is: %d\n\r", rtd);
    xil_printf("Celcius: %d\n\r", adc_to_celcius(adc));

    /* Same reading with one decimal place. */
    int c_x10 = adc_to_celcius_x10(adc);
    const char *sign = (c_x10 < 0 && c_x10 > -10) ? "-" : "";
    xil_printf("Temperature: %s%d.%d C\n\r",
               sign, c_x10 / 10, (c_x10 < 0 ? -c_x10 : c_x10) % 10);

    return 0;
}