/*
 * MAX31865 RTD reader -- N channels, one AXI Quad SPI core per channel.
 * Streams one CSV line per sample period, in tenths of a degree C.
 *
 *   256,301\r\n     both channels good
 *   256,\r\n        channel 1 did not answer -- empty field means missing
 *
 * Set USE_SECOND_CHANNEL to 0 to fall back to single-channel behaviour
 * without touching anything else.
 */
#include "xparameters.h"
#include "xspi.h"
#include "xil_printf.h"
#include "sleep.h"
#include "platform.h"

/* ------------------------------------------------------- build options */
#define USE_SECOND_CHANNEL 1   /* 0 = single channel, as before           */
#define DEBUG_MODE         0   /* 1 = diagnostics alongside the data      */
#define WAIT_FOR_HOST      0   /* 1 = block until the PC sends 'S'        */
#define SEND_HEADER        1   /* 1 = announce channel count and names    */
#define APPLY_BIT_SHIFT    0   /* only for automatic slave select         */

#define SAMPLE_PERIOD_US   1000000

#if DEBUG_MODE
  #define DBG(...) xil_printf(__VA_ARGS__)
#else
  #define DBG(...) ((void)0)
#endif

#if USE_SECOND_CHANNEL && !defined(XPAR_AXI_QUAD_SPI_1_BASEADDR)
  #error "No second SPI core in xparameters.h. Rebuild the block design, \
export the XSA, update the Vitis platform -- or set USE_SECOND_CHANNEL to 0."
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

#define CFG_STARTUP  (CFG_VBIAS_ON | CFG_AUTO_CONV | CFG_FAULT_CLEAR)

/* Manual slave select: the MAX31865 needs CS low >=400ns before the first
 * SCLK edge (tCC, p.4) and samples SCLK at the CS falling edge to detect
 * clock polarity (p.17). Automatic mode leaves no setup time for either. */
#define SPI_OPTIONS (XSP_MASTER_OPTION | XSP_CLK_ACTIVE_LOW_OPTION | \
                     XSP_CLK_PHASE_1_OPTION | XSP_MANUAL_SSELECT_OPTION)

/* --------------------------------------------------------- the channels */
/*
 * One row per sensor. Adding a third sensor is one more row -- no new
 * branches anywhere else in the file.
 *
 * You have two SPI cores with one sensor each, so every ss_mask is 0x1
 * and the BASE ADDRESS is what varies. If you later move to a shared bus
 * (one core, several chip selects) keep one base and vary ss_mask
 * instead: 0x1, 0x2, 0x4, 0x8 -- one-hot, one bit per sensor.
 */
typedef struct {
    u32         base;      /* XPAR_..._BASEADDR of this core   */
    u32         ss_mask;   /* one-hot chip select within it    */
    int         rref;      /* reference resistor, ohms         */
    const char *name;      /* header line and debug output     */
} channel_cfg_t;

static const channel_cfg_t CHANNELS[] = {
    { XPAR_AXI_QUAD_SPI_0_BASEADDR, 0x1, 400, "probe0" },
#if USE_SECOND_CHANNEL
    { XPAR_AXI_QUAD_SPI_1_BASEADDR, 0x1, 400, "probe1" },
#endif
};

#define N_CHANNELS ((int)(sizeof(CHANNELS) / sizeof(CHANNELS[0])))

static XSpi Spi[N_CHANNELS];

/* --------------------------------------------- conversions (pure math) */

/* RTD register: 15-bit code in D15..D1, fault flag in D0.
 * Read the fault bit before shifting -- the shift discards it. */
static int rtd_to_adc(u8 msb, u8 lsb, int *fault)
{
    if (fault) *fault = lsb & 0x01;
    return (msb << 7) | (lsb >> 1);
}

static int adc_to_rrtd(int adc, int rref)
{
    return (adc * rref) / 32768;
}

/* Linear approximation (p.11): T = code/32 - 256.
 * Returned in TENTHS of a degree so integer math keeps one decimal. */
static int adc_to_celsius_x10(int adc)
{
    return ((adc * 10) / 32) - 2560;
}

/* Out of reset, registers 03h/04h read FF FF and 05h/06h read 00 00.
 * With no sensor attached MISO floats, so the buffer comes back all 0x00
 * or all 0xFF and this test fails. That is how an unpopulated channel is
 * told apart from a cold one. */
static int looks_alive(const u8 *rx)
{
    return (rx[4] == 0xFF && rx[5] == 0xFF &&
            rx[6] == 0x00 && rx[7] == 0x00);
}

/* ------------------------------------------------------------- helpers */

#if APPLY_BIT_SHIFT
/* Realign data that arrives one bit late under automatic slave select.
 * Disabled: manual SS fixed the root cause, and applying this to already
 * aligned data would BREAK it. */
static void shift_buffer_left(u8 *buf, int len)
{
    for (int i = 0; i < len - 1; i++)
        buf[i] = (buf[i] << 1) | (buf[i + 1] >> 7);
    buf[len - 1] <<= 1;
}
#endif

#if DEBUG_MODE
static void dump_registers(int ch, const u8 *rx)
{
    xil_printf("ch%d got:", ch);
    for (int i = 0; i < REG_COUNT; i++) xil_printf(" %02X", rx[i]);
    xil_printf("\r\n     [1] 00h Config  : %02X"
               "\r\n     [2] 01h RTD MSB : %02X"
               "\r\n     [3] 02h RTD LSB : %02X"
               "\r\n     [4] 03h HFT MSB : %02X"
               "\r\n     [5] 04h HFT LSB : %02X"
               "\r\n     [6] 05h LFT MSB : %02X"
               "\r\n     [7] 06h LFT LSB : %02X"
               "\r\n     [8] 07h Fault   : %02X\r\n",
               rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7], rx[8]);
}
#endif

/* --------------------------------------------------- per-channel setup */

static int channel_init(int ch)
{
    const channel_cfg_t *c = &CHANNELS[ch];
    XSpi_Config *cfg;
    u8 tx[2] = { CONFIG_WRITE, CFG_STARTUP };
    u8 rx[2] = { 0 };

    cfg = XSpi_LookupConfig(c->base);
    if (!cfg) return XST_FAILURE;

    if (XSpi_CfgInitialize(&Spi[ch], cfg, cfg->BaseAddress) != XST_SUCCESS)
        return XST_FAILURE;

    XSpi_SetOptions(&Spi[ch], SPI_OPTIONS);
    XSpi_SetSlaveSelect(&Spi[ch], c->ss_mask);
    XSpi_Start(&Spi[ch]);          /* without Start the core stays inhibited */

    /* Force polled operation. Otherwise Transfer() returns immediately
     * having only started an interrupt-driven transfer that nothing
     * services, and rx[] stays zero while status reports success. */
    XSpi_IntrGlobalDisable(&Spi[ch]);

    /* Out of reset the config register is 00h -- bias off, ADC off -- so
     * the RTD registers never fill in. Add CFG_3WIRE for a 3-wire RTD. */
    return XSpi_Transfer(&Spi[ch], tx, rx, 2);
}

/* Tenths of a degree C. Sets *ok to 0 if the channel did not answer. */
static int channel_read(int ch, int *ok)
{
    u8 tx[REG_COUNT] = { CONFIG_READ };
    u8 rx[REG_COUNT] = { 0 };
    int fault = 0, adc;

    *ok = 0;

    if (XSpi_Transfer(&Spi[ch], tx, rx, REG_COUNT) != XST_SUCCESS) {
        DBG("ch%d: transfer failed\r\n", ch);
        return 0;
    }

#if APPLY_BIT_SHIFT
    shift_buffer_left(rx, REG_COUNT);
#endif
#if DEBUG_MODE
    {
        static int dumped[N_CHANNELS];      /* first read of each channel */
        if (!dumped[ch]) { dump_registers(ch, rx); dumped[ch] = 1; }
    }
#endif

    if (!looks_alive(rx)) {
        DBG("ch%d: no sensor -- [4..7] = %02X %02X %02X %02X\r\n",
            ch, rx[4], rx[5], rx[6], rx[7]);
        return 0;
    }

    adc = rtd_to_adc(rx[2], rx[3], &fault);

    if (fault) {
        DBG("ch%d: RTD fault bit set, 07h = %02X\r\n", ch, rx[8]);
    }

    DBG("ch%d %s: adc=%d rtd=%d ohm\r\n", ch, CHANNELS[ch].name,
        adc, adc_to_rrtd(adc, CHANNELS[ch].rref));

    *ok = 1;
    return adc_to_celsius_x10(adc);
}

/* ---------------------------------------------------------------- main */

int main(void)
{
    int ch;

    init_platform();
    DBG("\r\nM13 starting -- %d channel(s)\r\n", N_CHANNELS);

    for (ch = 0; ch < N_CHANNELS; ch++) {
        if (channel_init(ch) != XST_SUCCESS) {
            xil_printf("ch%d init failed\r\n", ch);
            return XST_FAILURE;
        }
        DBG("ch%d (%s) configured, wrote %02X\r\n",
            ch, CHANNELS[ch].name, CFG_STARTUP);
    }

#if WAIT_FOR_HOST
    while (inbyte() != 'S') { ; }   /* the PC gives the starting gun */
#endif

#if SEND_HEADER
    /* Self-describing: the board, not Python, decides how many channels
     * there are. Lines starting with '#' are metadata, not data. */
    xil_printf("#CH=%d", N_CHANNELS);
    for (ch = 0; ch < N_CHANNELS; ch++) xil_printf(",%s", CHANNELS[ch].name);
    xil_printf("\r\n");
#endif

    while (1) {
        usleep(SAMPLE_PERIOD_US);

        for (ch = 0; ch < N_CHANNELS; ch++) {
            int ok;
            int t_x10 = channel_read(ch, &ok);

            if (ok) xil_printf("%d", t_x10);   /* else the field stays empty */

            if (ch == N_CHANNELS - 1) xil_printf("\r\n");
            else                      xil_printf(",");
        }
    }

    return 0;
}