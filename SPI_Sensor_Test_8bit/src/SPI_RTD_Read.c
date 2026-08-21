/*
 * MAX31865 RTD reader -- AXI Quad SPI (PL core), Zynq-7000.
 * Streams temperature over UART as tenths of a degree C, once per second.
 */
#include "xparameters.h"
#include "xspi.h"
#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"

/* ------------------------------------------------------- build options */
#define DEBUG_MODE        0   /* 1 = diagnostics on the UART alongside data  */
#define WAIT_FOR_HOST     0   /* 1 = block until the PC sends 'S'            */
#define APPLY_BIT_SHIFT   0   /* only needed with automatic slave select     */

#define SAMPLE_PERIOD_US  1000000

#if DEBUG_MODE
  #define DBG(...) xil_printf(__VA_ARGS__)
#else
  #define DBG(...) ((void)0)
#endif

/* ------------------------------------------------------------ MAX31865 */
#define REG_COUNT      9      /* 1 address byte + registers 00h..07h */
#define CONFIG_READ    0x00
#define CONFIG_WRITE   0x80   /* write address = read address | 0x80 */

#define CFG_VBIAS_ON     0x80
#define CFG_AUTO_CONV    0x40
#define CFG_ONE_SHOT     0x20
#define CFG_3WIRE        0x10  /* 0 = 2- or 4-wire */
#define CFG_FAULT_CLEAR  0x02
#define CFG_50HZ         0x01  /* 0 = 60 Hz notch */

/* Manual slave select: the MAX31865 needs CS low >=400ns before the first
 * SCLK edge (tCC, p.4) and samples SCLK at the CS falling edge to detect
 * clock polarity (p.17). Automatic mode starts on the first SCK edge and
 * leaves no setup time for either. CPHA=1 required; CPOL is auto-detected. */
#define SPI_OPTIONS (XSP_MASTER_OPTION | XSP_CLK_ACTIVE_LOW_OPTION | \
                     XSP_CLK_PHASE_1_OPTION | XSP_MANUAL_SSELECT_OPTION)

static XSpi Spi;

/* --------------------------------------------- conversions (pure math) */

/* RTD register: 15-bit code in D15..D1, fault flag in D0.
 * Read the fault bit before shifting -- the shift discards it. */
static int rtd_to_adc(u8 msb, u8 lsb, int *fault)
{
    if (fault) *fault = lsb & 0x01;
    return (msb << 7) | (lsb >> 1);
}

/* Rref = 400 ohm. */
static int adc_to_rrtd(int adc)
{
    return (adc * 400) / 32768;
}

/* Linear approximation (p.11): T = code/32 - 256.
 * Returned in TENTHS of a degree so integer math keeps one decimal. */
static int adc_to_celsius_x10(int adc)
{
    return ((adc * 10) / 32) - 2560;
}

/* ------------------------------------------------------------- helpers */

#if APPLY_BIT_SHIFT
/* Realign data that arrives one bit late under automatic slave select.
 * Byte positions are unchanged. Disabled: manual SS fixed the root cause,
 * and applying this now would BREAK correctly aligned data. */
static void shift_buffer_left(u8 *buf, int len)
{
    for (int i = 0; i < len - 1; i++)
        buf[i] = (buf[i] << 1) | (buf[i + 1] >> 7);
    buf[len - 1] <<= 1;
}
#endif

#if DEBUG_MODE
/* Check against Table 1. Sanity: [4][5] must be FF FF, [6][7] must be
 * 00 00. Those are power-on constants -- if they are wrong, the byte
 * alignment is wrong and nothing below can be trusted. */
static void dump_registers(const u8 *rx)
{
    xil_printf("got:");
    for (int i = 0; i < REG_COUNT; i++) xil_printf(" %02X", rx[i]);
    xil_printf("\r\n [1] 00h Config   : %02X"
               "\r\n [2] 01h RTD MSB  : %02X"
               "\r\n [3] 02h RTD LSB  : %02X"
               "\r\n [4] 03h HFT MSB  : %02X"
               "\r\n [5] 04h HFT LSB  : %02X"
               "\r\n [6] 05h LFT MSB  : %02X"
               "\r\n [7] 06h LFT LSB  : %02X"
               "\r\n [8] 07h Fault    : %02X\r\n",
               rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7], rx[8]);
}
#endif

static int spi_init(void)
{
    XSpi_Config *cfg = XSpi_LookupConfig(XPAR_XSPI_0_BASEADDR);
    if (cfg == NULL) return XST_FAILURE;
    return XSpi_CfgInitialize(&Spi, cfg, cfg->BaseAddress);
}

/* ---------------------------------------------------------------- main */

int main(void)
{
    u8 cfg_tx[2] = { CONFIG_WRITE,
                     CFG_VBIAS_ON | CFG_AUTO_CONV | CFG_FAULT_CLEAR };
    u8 cfg_rx[2] = { 0 };
#if DEBUG_MODE
    int first = 1;
#endif

    init_platform();
    DBG("\r\nM13 starting (MAX31865)\r\n");

    if (spi_init() != XST_SUCCESS) {
        xil_printf("SPI init failed\r\n");
        return XST_FAILURE;
    }

    XSpi_SetOptions(&Spi, SPI_OPTIONS);
    XSpi_SetSlaveSelect(&Spi, 0x1);
    XSpi_Start(&Spi);            /* without Start the core stays inhibited */

    /* Force polled operation. Otherwise Transfer() returns immediately
     * having only kicked off an interrupt-driven transfer that nothing
     * services, and rx[] stays zero while status still reports success. */
    XSpi_IntrGlobalDisable(&Spi);

    /* Out of reset the config register is 00h -- bias off, ADC off -- so
     * the RTD registers never fill in. Add CFG_3WIRE for a 3-wire RTD. */
    XSpi_Transfer(&Spi, cfg_tx, cfg_rx, 2);
    DBG("Config wrote %02X\r\n", cfg_tx[1]);

#if WAIT_FOR_HOST
    while (inbyte() != 'S') { ; }   /* PC gives the starting gun */
#endif

    while (1) {
        u8 tx[REG_COUNT] = { CONFIG_READ };
        u8 rx[REG_COUNT] = { 0 };
        int adc, fault;

        usleep(SAMPLE_PERIOD_US);

        /* The chip auto-increments, so rx[1]..rx[8] hold 00h..07h. */
        if (XSpi_Transfer(&Spi, tx, rx, REG_COUNT) != XST_SUCCESS) {
            DBG("Transfer failed\r\n");
            continue;
        }

#if APPLY_BIT_SHIFT
        shift_buffer_left(rx, REG_COUNT);
#endif
#if DEBUG_MODE
        if (first) { dump_registers(rx); first = 0; }
#endif

        adc = rtd_to_adc(rx[2], rx[3], &fault);   /* rx[1] is 00h */

        DBG("adc=%d rtd=%d ohm%s\r\n", adc, adc_to_rrtd(adc),
            fault ? "  FAULT (check 07h)" : "");

        /* Wire format: tenths of a degree, plain integer, nothing else.
         * Python side: value = int(line) / 10.0 */
        xil_printf("%d\r\n", adc_to_celsius_x10(adc));
    }

    return 0;
}