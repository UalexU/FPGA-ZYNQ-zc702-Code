"""
board.py -- which board is on the other end of the serial cable.

The host code is shared between the Cmod S7 (MicroBlaze) build and the
Trenz TE0745 (Zynq-7000) build. The serial protocol is identical; what
differs is the UART, the SPI clock, whether the MAX31865 is fitted, and
the words on the screen. All of that lives here and nowhere else.

Pick the board, highest priority first:
    python tmag_scope.py --board cmod_s7
    set TMAG_BOARD=cmod_s7          (environment variable)
    DEFAULT below                   (this folder is the TE0745 copy)
"""

import os
from dataclasses import dataclass, field

DEFAULT = "te0745"


@dataclass(frozen=True)
class Board:
    key: str
    name: str                 # full name, for titles and the assistant
    short: str                # status bar / window title
    cpu: str
    uart: str                 # what the firmware's stdout is
    baud: int
    baud_note: str            # where the baud is actually set
    uart_fifo_bytes: int      # TX FIFO: bytes xil_printf writes without blocking
    sck_khz: int              # SPI_SCK_KHZ in main.c, used until CONFIG arrives
    spi_call_overhead_us: float
    has_rtd: bool             # MAX31865 wired and read by the firmware
    firmware: str             # main.c, relative to the project root
    read_roots: tuple = field(default_factory=tuple)   # for the assistant
    # USB-UART bridges worth preferring when guessing the COM port.
    port_hints: tuple = ("FTDI", "FT2232", "FT232", "Future Technology",
                         "USB Serial")

    @property
    def spi_bytes_per_sample(self):
        # four 32-bit TMAG5170 frames, plus the MAX31865 9-byte burst if read
        return 4 * 4 + (9 if self.has_rtd else 0)


PROFILES = {
    "te0745": Board(
        key="te0745",
        name="Trenz TE0745-03 (Zynq XC7Z045) on TEB0745",
        short="TE0745",
        cpu="Zynq-7000 PS, Cortex-A9 bare metal",
        uart="Zynq PS UART",
        baud=115200,
        baud_note=("PS UART baud: set in the Vitis platform "
                   "(standalone stdin/stdout baud), not in Vivado"),
        uart_fifo_bytes=64,           # Cadence UART TX FIFO
        sck_khz=3125,                 # 50 MHz ext_spi_clk / 16
        spi_call_overhead_us=8.0,     # A9 @ ~667 MHz, polled XSpi -- estimate
        has_rtd=True,                 # RTD_ONLY 1 build: MAX31865 fitted
                                      # (the GUI decides per channel from
                                      # the data; this only seeds defaults)
        firmware="fw/SENSORS/src/main.c",
        read_roots=(
            "fw/SENSORS/src",
            "hw/TE0745-03-92I31-A.srcs/constrs_1",
            "hw/TE0745-03-92I31-A.srcs/sources_1/bd/SPI_TMAG5170UEVM_design_1/"
            "SPI_TMAG5170UEVM_design_1.bd",
        ),
    ),
    "cmod_s7": Board(
        key="cmod_s7",
        name="Digilent Cmod S7-25 (MicroBlaze)",
        short="Cmod S7",
        cpu="MicroBlaze soft core",
        uart="AXI Uartlite",
        baud=115200,
        baud_note="AXI Uartlite baud: fixed at synthesis in Vivado",
        uart_fifo_bytes=16,
        sck_khz=625,
        spi_call_overhead_us=40.0,
        has_rtd=True,
        firmware="fw/SPI_BOTH/src/main.c",
        read_roots=(
            "fw/SPI_BOTH/src",
            "fw/SPI_Hall_Temp/src",
            "fw/SPI_Hull_Sensor/src",
            "hw/CMOD_S7 FPGA.srcs/constrs_1",
            "hw/CMOD_S7 FPGA.srcs/sources_1/bd/design_1/design_1.bd",
        ),
    ),
}

_env = os.environ.get("TMAG_BOARD", DEFAULT).strip().lower()
ACTIVE = PROFILES.get(_env, PROFILES[DEFAULT])


def select(key):
    """Switch profile. Call before importing anything that reads ACTIVE at
    import time (sensor, instrument.spec) -- tmag_scope does this from
    --board before its own imports."""
    global ACTIVE
    key = (key or DEFAULT).strip().lower()
    if key not in PROFILES:
        raise SystemExit(f"unknown board {key!r}; choose from "
                         f"{', '.join(PROFILES)}")
    ACTIVE = PROFILES[key]
    os.environ["TMAG_BOARD"] = key
    return ACTIVE


def board_from_argv(argv):
    """Peek at --board in argv without a full parse, so modules that read
    ACTIVE at import time see the right one."""
    for i, arg in enumerate(argv):
        if arg == "--board" and i + 1 < len(argv):
            return select(argv[i + 1])
        if arg.startswith("--board="):
            return select(arg.split("=", 1)[1])
    return ACTIVE
