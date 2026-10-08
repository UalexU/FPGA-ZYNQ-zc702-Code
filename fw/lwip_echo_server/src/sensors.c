/*
 * sensors.c -- TMAG5170 (mode 0) and MAX31865 (mode 3) on AXI Quad SPI.
 * Logic unchanged from the UART-only main.c; text now goes through out.c.
 *
 * Vivado, per core: 1 slave, 8-bit transaction width, STARTUP primitive
 * off, ext_spi_clk and s_axi_aresetn connected, AND AN ASSIGNED ADDRESS.
 */

#include <stddef.h>

#include "sensors.h"
#include "app_config.h"
#include "out.h"

#include "xparameters.h"
#include "xspi.h"
#include "xil_io.h"
#include "sleep.h"
#if defined(__has_include)
#  if __has_include("xiic_l.h")
#    include "xiic_l.h"
#    define HAVE_XIIC 1
#  endif
#endif

/* ===================== SPI cores ===================== */

#define SPI_BASE_TMAG   XPAR_AXI_QUAD_SPI_0_BASEADDR
#if HAVE_RTD
#  if RTD_ONLY
#    define SPI_BASE_RTD XPAR_AXI_QUAD_SPI_0_BASEADDR
#  else
#    ifndef XPAR_AXI_QUAD_SPI_1_BASEADDR
#      error "ENABLE_RTD=1 needs a second AXI Quad SPI (axi_quad_spi_1) in Vivado, mode 3"
#    endif
#    define SPI_BASE_RTD XPAR_AXI_QUAD_SPI_1_BASEADDR
#  endif
#endif

#define SPI_CS          0x01
#define SPI_NO_CS       0x00

#define OPTS_TMAG   (XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION)
#define OPTS_RTD    (XSP_MASTER_OPTION | XSP_MANUAL_SSELECT_OPTION | \
                     XSP_CLK_ACTIVE_LOW_OPTION | XSP_CLK_PHASE_1_OPTION)

static XSpi SpiTmag;
#if HAVE_RTD
static XSpi SpiRtd;
#endif

/* ===================== TMAG5170 registers ===================== */

#define REG_DEVICE_CONFIG   0x00
#define REG_SENSOR_CONFIG   0x01
#define REG_X_CH_RESULT     0x09
#define REG_Y_CH_RESULT     0x0A
#define REG_Z_CH_RESULT     0x0B
#define REG_TEMP_RESULT     0x0C
#define CMD_DISABLE_CRC     0x0F000407  /* datasheet Sec 7.5.2.5 */
#define T_ADC_T0            17522
#define T_ADC_RES           60

static u8  conv_avg_code = CONV_AVG;
static u8  range_code    = RANGE_CODE;
static s32 range_mt_x100 = RANGE_MT_X100;

/* ===================== MAX31865 registers ===================== */

#define RTD_REG_COUNT       9       /* address byte + registers 00h..07h */
#define RTD_ADDR_READ       0x00
#define RTD_ADDR_WRITE      0x80
#define RTD_VBIAS_ON        0x80
#define RTD_AUTO_CONV       0x40
#define RTD_FAULT_CLEAR     0x02
#define RTD_CONFIG      (RTD_VBIAS_ON | RTD_AUTO_CONV | RTD_FAULT_CLEAR | \
                         RTD_WIRE_MODE | RTD_FILTER)
#define RTD_CONFIG_READBACK (RTD_CONFIG & ~RTD_FAULT_CLEAR)

/* ===================== SPI plumbing ===================== */

static void spi_dump(XSpi *dev, const char *label)
{
    (void)dev;
    (void)label;
    DBG("#   %s: base=0x%08X ss_bits=%d CR=0x%04X SR=0x%04X\r\n",
        label, (unsigned)dev->BaseAddr, dev->NumSlaveBits,
        (unsigned)XSpi_ReadReg(dev->BaseAddr, XSP_CR_OFFSET),
        (unsigned)XSpi_GetStatusReg(dev));
}

/* One transfer, CS held low throughout. */
static int spi_burst(XSpi *dev, const char *label, u8 *tx, u8 *rx, int len)
{
    int status;

    XSpi_SetSlaveSelect(dev, SPI_CS);
    status = XSpi_Transfer(dev, tx, rx, len);
    XSpi_SetSlaveSelect(dev, SPI_NO_CS);

    if (status != XST_SUCCESS) {
        out_printf("# %s: transfer failed (%d)\r\n", label, status);
    }
    return status;
}

/* Bring one core up, polled (interrupts off, or Transfer() returns early). */
static int spi_setup(XSpi *dev, u32 base, u32 opts, const char *label)
{
    XSpi_Config *cfg;
    int status;

#ifdef SDT
    cfg = XSpi_LookupConfig(base);
#else
    cfg = XSpi_LookupConfig(XPAR_AXI_QUAD_SPI_0_DEVICE_ID +
                            (base == XPAR_AXI_QUAD_SPI_0_BASEADDR ? 0 : 1));
#endif
    if (cfg == NULL) {
        out_printf("# %s: no config -- is the core addressed in Vivado?\r\n",
                   label);
        return XST_FAILURE;
    }
    status = XSpi_CfgInitialize(dev, cfg, cfg->BaseAddress);
    if (status != XST_SUCCESS) {
        out_printf("# %s: CfgInitialize failed (%d)\r\n", label, status);
        return status;
    }
    status = XSpi_SetOptions(dev, opts);
    if (status != XST_SUCCESS) {
        out_printf("# %s: SetOptions failed (%d)\r\n", label, status);
        return status;
    }

    XSpi_Start(dev);
    XSpi_IntrGlobalDisable(dev);
    XSpi_SetSlaveSelect(dev, SPI_NO_CS);
    spi_dump(dev, label);

    if (dev->DataWidth != XSP_DATAWIDTH_BYTE) {
        out_printf("# %s: transaction width %d bits, expected 8\r\n",
                   label, dev->DataWidth);
    }
    return XST_SUCCESS;
}

/* ===================== TMAG5170 ===================== */

static u32 tmag_frame(u8 rw, u8 addr, u16 data)
{
    return ((u32)(rw & 1) << 31) | ((u32)(addr & 0x7F) << 24) |
           ((u32)data << 8);
}

static u32 tmag_xfer(u32 frame)
{
    u8 tx[4], rx[4] = { 0 };

    tx[0] = (u8)(frame >> 24);
    tx[1] = (u8)(frame >> 16);
    tx[2] = (u8)(frame >> 8);
    tx[3] = (u8)frame;

    if (spi_burst(&SpiTmag, "tmag", tx, rx, 4) != XST_SUCCESS) {
        return 0;
    }
    return ((u32)rx[0] << 24) | ((u32)rx[1] << 16) |
           ((u32)rx[2] << 8) | rx[3];
}

static void tmag_write(u8 addr, u16 data)
{
    tmag_xfer(tmag_frame(0, addr, data));
}

static u16 tmag_read(u8 addr)
{
    return (u16)((tmag_xfer(tmag_frame(1, addr, 0)) >> 8) & 0xFFFF);
}

static s32 tmag_mT_x100(u16 raw)
{
    return ((s32)(s16)raw * range_mt_x100) / 32768;
}

static s32 tmag_degC_x100(u16 raw)
{
    return 2500 + (((s32)raw - T_ADC_T0) * 100) / T_ADC_RES;
}

static int tmag_verify(u8 addr, u16 want, const char *label)
{
    u16 got = tmag_read(addr);

    if (got != want) {
        out_printf("# %s readback 0x%04X, expected 0x%04X\r\n",
                   label, got, want);
        return 1;
    }
    return 0;
}

static u16 sensor_config_word(void)
{
    return (u16)(0x01C0 | ((u16)range_code << 4) |
                 ((u16)range_code << 2) | range_code);
}

static u16 device_config_word(void)
{
    return (u16)(((u16)conv_avg_code << 12) | 0x0020 | 0x0008 | 0x0004);
}

static int tmag_apply_sensor_config(void)
{
    u16 want = sensor_config_word();

    tmag_write(REG_SENSOR_CONFIG, want);
    return tmag_verify(REG_SENSOR_CONFIG, want, "SENSOR_CONFIG");
}

static int tmag_apply_device_config(void)
{
    u16 want = device_config_word();

    tmag_write(REG_DEVICE_CONFIG, want);
    usleep(1000);
    return tmag_verify(REG_DEVICE_CONFIG, want, "DEVICE_CONFIG");
}

/* ===================== MAX31865 ===================== */

#if HAVE_RTD
static void rtd_configure(void)
{
    u8 tx[2] = { RTD_ADDR_WRITE, RTD_CONFIG };
    u8 rx[2] = { 0 };

    spi_burst(&SpiRtd, "rtd cfg", tx, rx, 2);
}

/* regs[0] = config, [1..2] = RTD code, [3..6] = thresholds, [7] = fault. */
static int rtd_read_regs(u8 *regs)
{
    u8 tx[RTD_REG_COUNT] = { RTD_ADDR_READ };
    u8 rx[RTD_REG_COUNT] = { 0 };
    int i;

    if (spi_burst(&SpiRtd, "rtd", tx, rx, RTD_REG_COUNT) != XST_SUCCESS) {
        return -1;
    }
    for (i = 0; i < RTD_REG_COUNT - 1; i++) {
        regs[i] = rx[i + 1];
    }
    return 0;
}

/* Power-on thresholds are FF FF 00 00; config reads back what was written. */
static int rtd_check_link(void)
{
    u8 r[8];
    int bad = 0;

    if (rtd_read_regs(r) != 0) {
        return 1;
    }
    if (r[0] != RTD_CONFIG_READBACK) {
        out_printf("# RTD config readback 0x%02X, expected 0x%02X\r\n",
                   r[0], RTD_CONFIG_READBACK);
        bad = 1;
    }
    if (r[3] != 0xFF || r[4] != 0xFF || r[5] != 0x00 || r[6] != 0x00) {
        out_printf("# RTD thresholds %02X %02X %02X %02X, expected "
                   "FF FF 00 00 (power-on values)\r\n",
                   r[3], r[4], r[5], r[6]);
        bad = 1;
    }
    if (!bad) {
        out_printf("# RTD link ok\r\n");
    }
    return bad;
}

static void rtd_print_fault(u8 status)
{
    out_printf("# RTD FAULT 0x%02X:%s%s%s%s%s%s%s\r\n", status,
               (status & 0x80) ? " RTD-high-threshold" : "",
               (status & 0x40) ? " RTD-low-threshold" : "",
               (status & 0x20) ? " REFIN->0.85*VBIAS" : "",
               (status & 0x10) ? " REFIN-<0.85*VBIAS(FORCE-open)" : "",
               (status & 0x08) ? " RTDIN-<0.85*VBIAS(FORCE-open)" : "",
               (status & 0x04) ? " over/under-voltage" : "",
               (status == 0)   ? " (flag set, status clear)" : "");
}

static s32 rtd_mohm(u16 code)
{
    s64 r = ((s64)code * RTD_RREF_MOHM) / 32768;

    return (s32)(r - RTD_LEAD_MOHM);
}

static s32 rtd_degC_x100_from_mohm(s32 r_mohm)
{
    s64 slope = ((s64)RTD_R0_MOHM * RTD_ALPHA_PPM) / 1000000; /* mohm/C */

    return (s32)((((s64)r_mohm - RTD_R0_MOHM) * 100) / slope);
}
#endif /* HAVE_RTD */

/* ===================== I2C scan (optional) ===================== */

#if ENABLE_I2C_SCAN && defined(HAVE_XIIC) && defined(XPAR_AXI_IIC_0_BASEADDR)
static void i2c_scan(void)
{
    u8 buf;
    int addr, found = 0;

    out_printf("# I2C scan on axi_iic_0:");
    for (addr = 0x08; addr < 0x78; addr++) {
        if (XIic_Recv(XPAR_AXI_IIC_0_BASEADDR, (u8)addr, &buf, 1,
                      XIIC_STOP) == 1) {
            out_printf(" 0x%02X", addr);
            found++;
        }
    }
    out_printf(found ? "\r\n" : " none\r\n");
}
#else
static void i2c_scan(void) { }
#endif

/* Exact integer sqrt -- no libm. */
static u32 isqrt_u32(u32 n)
{
    u32 rem = 0, root = 0;
    int i;

    for (i = 0; i < 16; i++) {
        root <<= 1;
        rem = (rem << 2) | (n >> 30);
        n <<= 2;
        if (root < rem) {
            root++;
            rem -= root;
            root++;
        }
    }
    return root >> 1;
}

/* ===================== public ===================== */

int sensors_init(void)
{
    if (HAVE_TMAG) {
        if (spi_setup(&SpiTmag, SPI_BASE_TMAG, OPTS_TMAG, "tmag core")
                != XST_SUCCESS) {
            return -1;
        }
    }
#if HAVE_RTD
    if (spi_setup(&SpiRtd, SPI_BASE_RTD, OPTS_RTD, "rtd core")
            != XST_SUCCESS) {
        return -1;
    }
#  if !RTD_ONLY
    if (SpiTmag.BaseAddr == SpiRtd.BaseAddr) {
        out_printf("# BOTH CORES AT 0x%08X -- check xparameters.h\r\n",
                   (unsigned)SpiTmag.BaseAddr);
        return -1;
    }
#  endif
#endif

    i2c_scan();

    if (HAVE_TMAG) {
        tmag_xfer(CMD_DISABLE_CRC);
        usleep(1000);
        tmag_apply_sensor_config();
        tmag_apply_device_config();
    }

#if HAVE_RTD
    out_printf("# RTD %s on axi_quad_spi_%d, %s-wire, R0=%d ohm, "
               "Rref=%d ohm, lead=%d mohm, %d Hz notch\r\n",
               RTD_ONLY ? "only" : "+TMAG", RTD_ONLY ? 0 : 1,
               (RTD_WIRE_MODE & 0x10) ? "3" : "2/4",
               RTD_R0_MOHM / 1000, RTD_RREF_MOHM / 1000, RTD_LEAD_MOHM,
               RTD_CONV_HZ);
    rtd_configure();
    usleep(100000);         /* first auto conversion: ~66 ms (60 Hz) */
    rtd_check_link();
#else
    usleep(20000);          /* let the TMAG finish its first conversion set */
#endif
    return 0;
}

void sensors_read(SensorSample *s)
{
    s->bx = s->by = s->bz = s->mag = s->dieC = 0;
    s->rtdC = s->rtd_mohm = 0;
    s->rtd_fault = 1;

    if (HAVE_TMAG) {
        u16 rawX = tmag_read(REG_X_CH_RESULT);
        u16 rawY = tmag_read(REG_Y_CH_RESULT);
        u16 rawZ = tmag_read(REG_Z_CH_RESULT);
        u16 rawT = tmag_read(REG_TEMP_RESULT);

        s->bx = tmag_mT_x100(rawX);
        s->by = tmag_mT_x100(rawY);
        s->bz = tmag_mT_x100(rawZ);
        s->dieC = tmag_degC_x100(rawT);
        s->mag = (s32)isqrt_u32((u32)(s->bx * s->bx) + (u32)(s->by * s->by) +
                                (u32)(s->bz * s->bz));
    }

#if HAVE_RTD
    {
        u8 regs[8];

        if (rtd_read_regs(regs) == 0) {
            u16 code = (u16)((((u16)regs[1] << 8) | regs[2]) >> 1);

            s->rtd_fault = regs[2] & 0x01;     /* D0 of the LSB */
            s->rtd_mohm  = rtd_mohm(code);
            s->rtdC      = rtd_degC_x100_from_mohm(s->rtd_mohm);

            if (s->rtd_fault) {
                rtd_print_fault(regs[7]);
                rtd_configure();               /* clear the latch */
            }
        }
    }
#endif
}

int sensors_set_avg(int mult)
{
    int code = 0;

    if (!HAVE_TMAG) {
        return -3;
    }
    if (mult < 1 || mult > 32) {
        return -1;
    }
    while ((mult & 1) == 0) {
        mult >>= 1;
        code++;
    }
    if (mult != 1) {
        return -1;                     /* not a power of two */
    }
    conv_avg_code = (u8)code;
    return tmag_apply_device_config() ? -2 : 0;
}

int sensors_set_range(int mt)
{
    if (!HAVE_TMAG) {
        return -3;
    }
    if (mt == 25) {
        range_code = 0x1;
    } else if (mt == 50) {
        range_code = 0x0;
    } else if (mt == 100) {
        range_code = 0x2;
    } else {
        return -1;
    }
    range_mt_x100 = mt * 100;
    return tmag_apply_sensor_config() ? -2 : 0;
}

int sensors_avg_mult(void)
{
    return 1 << conv_avg_code;
}

int sensors_range_mt(void)
{
    return (int)(range_mt_x100 / 100);
}
