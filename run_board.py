"""
Load the bitstream + ELF onto the Zynq over JTAG, from Python.

This does NOT run your C code. It drives XSDB (the same engine Vitis
uses behind the Run button) and tells it to push files onto the board.

Usage:
    python run_board.py              # bitstream + ELF  (after power-on)
    python run_board.py --fast       # ELF only         (code-only rebuild)

Close any active Vitis debug session first -- only one program can own
the JTAG cable at a time.
"""

import argparse
import pathlib
import subprocess
import sys
import tempfile

# --------------------------------------------------------------- paths
VITIS_BIN = pathlib.Path(r"C:\Xilinx\2025.1\Vitis\bin")
WS        = pathlib.Path(r"C:\Users\Admin\XilinxProjects\Test\Vitis")

APP  = WS / "SPI_Sensor_Test_8bit"
BIT  = APP / "_ide" / "bitstream" / "design_1_wrapper.bit"
PS7  = APP / "_ide" / "psinit" / "ps7_init.tcl"
ELF  = APP / "build" / "SPI_Sensor_Test_8bit.elf"

TARGET = "*Cortex-A9*#0"        # Zynq-7000, first ARM core


def find_xsdb():
    """2025.1 ships xsdb.bat; older versions call it xsct.bat."""
    for name in ("xsdb.bat", "xsct.bat"):
        candidate = VITIS_BIN / name
        if candidate.exists():
            return candidate
    sys.exit(f"No xsdb.bat or xsct.bat found in {VITIS_BIN}")


def build_tcl(skip_bitstream):
    """The same steps Vitis performs, written out explicitly."""
    lines = [
        "connect",
        f'targets -set -nocase -filter {{name =~ "{TARGET}"}}',
        "rst -system",
        "after 3000",
        f'targets -set -nocase -filter {{name =~ "{TARGET}"}}',

        # 1. configure the PS: clocks, DDR, MIO pin routing
        f'source "{PS7.as_posix()}"',
        "ps7_init",
        "ps7_post_config",
    ]

    # 2. build the SPI controller in the PL (the slow ~4 MB step)
    if not skip_bitstream:
        lines.append(f'fpga -file "{BIT.as_posix()}"')

    # 3. load your program and start it
    lines += [
        f'targets -set -nocase -filter {{name =~ "{TARGET}"}}',
        f'dow "{ELF.as_posix()}"',
        "con",
        "exit",
    ]
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fast", action="store_true",
                    help="skip the bitstream (only safe if the board has not "
                         "lost power since it was last configured)")
    args = ap.parse_args()

    for f in ([PS7, ELF] if args.fast else [PS7, BIT, ELF]):
        if not f.exists():
            sys.exit(f"Missing: {f}")

    xsdb = find_xsdb()
    tcl = build_tcl(args.fast)
    print(f"--- script ---\n{tcl}--------------")

    with tempfile.NamedTemporaryFile("w", suffix=".tcl", delete=False) as fh:
        fh.write(tcl)
        tcl_path = fh.name

    try:
        result = subprocess.run([str(xsdb), tcl_path],
                                capture_output=True, text=True, timeout=180)
        print(result.stdout)
        if result.returncode != 0:
            print(result.stderr, file=sys.stderr)
            sys.exit("XSDB failed -- see output above")
        print("Board is running.")
    finally:
        pathlib.Path(tcl_path).unlink(missing_ok=True)


if __name__ == "__main__":
    main()
